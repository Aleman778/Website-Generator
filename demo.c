#include "generator.h"


typedef union {
    struct {
        string stylesheet_path;
        string script_path;
        string content;
    };
    string data[3];
    
} Template_Parameters;


int
main(int argc, char* argv[]) {
    char* filename = "hello_world.md";
    Dom dom = read_markdown_file(filename);
    string html = generate_html_from_dom(&dom);
    //printf("Generated:\n%.*s\n", (int) html.count, html.data);
    
    Template_Parameters params;
    params.stylesheet_path = string_lit("assets/style.css");
    params.script_path = string_lit("assets/script.js");
    params.content = html;
    
    string template_html = read_entire_file("base_template.html");
    string result = template_process_string(template_html, array_count(params.data), params.data);
    printf("Generated:\n%.*s\n", (int) result.count, result.data);
    write_entire_file("generated.html", result);
}