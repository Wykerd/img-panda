#include "img-panda/comics/identify/wordpress.h"
#include "simdjson.h"

int imp_parse_wordpress_site_info_json (imp_wc_meta_index_t *page, imp_buf_t *json) {
    if ((page == NULL) || (json == NULL)) {
        puts("!!! Warning: nullptr passed to parse_wordpress_site_info_json");
        return 0;
    }
    simdjson::dom::parser parser;
    simdjson::dom::element wp_json;
    auto error = parser.parse(json->base, json->len).get(wp_json);
    if (error) {
        return 0;
    }

    auto site_name_value = wp_json["name"];
    auto site_desc_value = wp_json["description"];

    if (site_name_value.error()) {
        std::cout << "!!! WordPress API: Failed to fetch name: " << site_name_value.error();
    } else {
        auto site_name = site_name_value.get_string().value();
        if (site_name.length() > 0) {
            if (page->site_title.data.len < site_name.length())
                page->site_title.data.base = (char *)realloc(page->site_title.data.base, site_name.length());

            page->site_title.data.len = site_name.length();
            memcpy(page->site_title.data.base, site_name.begin(), site_name.length());
            page->site_title.priority = -1;
            std::cout << "Identified Site Name: " << site_name << std::endl;
        }
    };

    if (site_desc_value.error()) {
        std::cout << "!!! WordPress API: Failed to fetch description: " << site_desc_value.error();
    } else {
        auto site_desc = site_desc_value.get_string().value();
        if (site_desc.length() > 0) {
            if (page->site_desc.data.len < site_desc.length())
                page->site_desc.data.base = (char *)realloc(page->site_desc.data.base, site_desc.length());

            page->site_desc.data.len = site_desc.length();
            memcpy(page->site_desc.data.base, site_desc.begin(), site_desc.length());
            page->site_desc.priority = -1;
            std::cout << "Identified Comic Description: " << site_desc << std::endl;
        }
    };

    return 1;
}
