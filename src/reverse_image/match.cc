#include <iostream>
#include "img-panda/common.h"
#include "img-panda/reverse_image/db.hpp"
#include "img-panda/reverse_image/match.h"
#include <opencv2/core.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <unordered_map>

// Internal state
typedef struct imp_ri_intr_state_s {
    imp_ri_imgs_t imgs;
    cv::FlannBasedMatcher matcher;
} imp_ri_intr_state_t;

inline
static cv::Mat imp__ri_load_grayscale (imp_buf_t *buf, size_t max_area = -1) {
    std::vector<char> imgbuf(buf->base, buf->base + buf->len);
    cv::Mat src = cv::imdecode(imgbuf, cv::IMREAD_GRAYSCALE);
    if (src.empty()) return src;
    size_t current_area = src.size().area();
    if (current_area > max_area) {
        cv::Mat ret;
        double devider = sqrt((max_area * 1.0) / current_area);
        cv::resize(src, ret, cv::Size(), devider, devider);
        return ret;
    };
    return src;
};

static imp_ri_kpdesc_t imp__ri_compute_descriptor (cv::Mat &src) {
    cv::Ptr<cv::BRISK> detector = cv::BRISK::create(80);
    std::vector<cv::KeyPoint> kp;
    cv::Mat desc;
    detector->detectAndCompute(src, cv::noArray(), kp, desc);
    imp_ri_kpdesc_t ret = {
        .desc = desc,
        .kp = kp
    };
    return ret;
};

inline
static int imp__ri_add_img (imp_ri_intr_state_t *state, cv::Mat &src, const char* uri, imp_db_t *db) {
    if (src.empty()) return 0;

    auto desc = imp__ri_compute_descriptor(src);

    state->imgs.matrix.push_back(desc.desc);
    state->imgs.uris.push_back(std::string(uri));

    size_t id = state->imgs.ids_len > 0 ? state->imgs.ids[state->imgs.ids_len - 1] + 1 : 1;
    
    state->imgs.ids = (int *)realloc(state->imgs.ids, state->imgs.matrix.rows * sizeof(int));
    if (state->imgs.ids == NULL) return 0;
    for (int i = state->imgs.ids_len; i < state->imgs.matrix.rows; i++) {
        state->imgs.ids[i] = id;
    };
    state->imgs.ids_len = state->imgs.matrix.rows;

    return imp_ri_db_add_img(db, desc.desc, uri);
};

int imp_ri_add_img (imp_ri_state_t *state, imp_buf_t *img, const char *uri) {
    cv::Mat mat = imp__ri_load_grayscale(img, state->max_area);

    return imp__ri_add_img (static_cast<imp_ri_intr_state_t *>(state->data), mat, uri, state->db);
};

static bool imp__ri_freq_compare(std::pair<size_t, size_t> p1, std::pair<size_t, size_t> p2) {
    if (p1.second == p2.second)
        return p1.first > p2.first;

    return p1.second > p2.second;
}

inline
static imp_ri_matches_t *imp__ri_match_img (imp_ri_state_t *state, cv::Mat &mat, size_t k) {
    auto desc = imp__ri_compute_descriptor(mat);

    imp_ri_intr_state_t *intr_state = static_cast<imp_ri_intr_state_t *>(state->data);

    std::vector< std::vector<cv::DMatch> > knn_matches;
    intr_state->matcher.knnMatch(intr_state->imgs.matrix, desc.desc, knn_matches, 2);

    const float ratio_thresh = 0.6f;

    std::unordered_map<size_t, size_t> freq;

    for (size_t i = 0; i < knn_matches.size(); i++) 
        if ((knn_matches[i].size() > 1) && (knn_matches[i][0].distance < ratio_thresh * knn_matches[i][1].distance))
            freq[intr_state->imgs.ids[i]]++;

    std::vector<std::pair<size_t, size_t>> freq_vec(freq.begin(), freq.end());

    std::sort(freq_vec.begin(), freq_vec.end(), imp__ri_freq_compare);

    size_t freq_len = freq_vec.size();
    size_t kk = k > freq_len ? freq_len : k;

    if (kk == 0) return NULL;

    imp_ri_matches_t *kmatch = (imp_ri_matches_t *)malloc(sizeof(imp_ri_matches_t));
    if (kmatch == NULL) return NULL;

    kmatch->idx = (imp_ri_match_pair_t *)malloc(kk * sizeof(imp_ri_match_pair_t));
    if (kmatch->idx == NULL) return NULL;
    kmatch->len = kk;

    for (size_t i = 0; i < kk; i++) {
        kmatch->idx[i].id = freq_vec[i].first;
        kmatch->idx[i].freq = freq_vec[i].second;
    };

    return kmatch;
}

imp_ri_matches_t *imp_ri_match_img (imp_ri_state_t *state, imp_buf_t *img, size_t k) {
    cv::Mat mat = imp__ri_load_grayscale(img, state->max_area);

    return imp__ri_match_img (state, mat, k);
};

void imp_ri_match_free (imp_ri_matches_t *matches) {
    free(matches->idx);
    free(matches);
};

int imp_ri_state_load (imp_ri_state_t *state) {
    imp_ri_intr_state_t *intr_s = static_cast<imp_ri_intr_state_t *>(state->data);
    if (!imp_ri_db_get_imgs(state->db, &intr_s->imgs)) return 0;
    return 1;
};

int imp_ri_state_init (imp_ri_state_t *state) {
    try
    {
        imp_ri_intr_state_t *intr_state = new imp_ri_intr_state_t;
        intr_state->matcher = cv::FlannBasedMatcher(cv::makePtr<cv::flann::LshIndexParams>(12, 20, 2));
        
        intr_state->imgs.ids = (int *)malloc(sizeof(int));
        if (intr_state->imgs.ids == NULL) {
            delete intr_state;
            return 0;
        };
        intr_state->imgs.ids_len = 0;
        state->data = (void *)intr_state;
        state->max_area = -1;
        return 1;
    }
    catch(const std::exception& e)
    {
        return 0;
    };
};

int imp_ri_state_verify_integrity (imp_ri_state_t *state) {
    return static_cast<imp_ri_intr_state_t *>(state->data)->imgs.ids != NULL;
};

void imp_ri_state_free (imp_ri_state_t *state) {
    free(static_cast<imp_ri_intr_state_t *>(state->data)->imgs.ids);
    delete (static_cast<imp_ri_intr_state_t *>(state->data));
};

/*
int main () {
    cv::Mat in = cv::imread("../im2.png", cv::IMREAD_GRAYSCALE );
    auto current_area = in.size().area();
    double devider = sqrt((512 * 512.0) / current_area);
    std::cout << current_area << " " << devider << std::endl;
    cv::Mat src;
    cv::resize(in, src, cv::Size(), devider, devider);
    auto desc = imp__ri_compute_descriptor(src);

    imp_ri_state_t state;
    imp_db_t db;
    imp_ri_state_init(&state);
    imp_db_sqlite_connect(&db, "../bank.db");
    imp_db_sqlite_init (&db);
    state.db = &db;
    imp_ri_state_load(&state);
    state.max_area = 512 * 512;
    //imp__ri_add_img((imp_ri_intr_state_t *)state.data, src, "https://google.com/", state.db);


    cv::Mat in2 = cv::imread("../im2_pp.png", cv::IMREAD_GRAYSCALE );
    auto current_area2 = in2.size().area();
    double devider2 = 1;//sqrt((512 * 512.0) / current_area2);
    std::cout << current_area2 << " " << devider2 << std::endl;
    cv::Mat src2;
    cv::resize(in2, src2, cv::Size(), devider2, devider2);
    auto desc2 = imp__ri_compute_descriptor(src2);

    imp_ri_matches_t * im = imp__ri_match_img(&state, src2, 3);
}
*/