#include "generator.c"



int
main(int argc, char* argv[]) {
    char* filename = "hello_world.md";
    Dom dom = read_markdown_file(filename);
    string html = generate_html_from_dom(&dom);
    write_entire_file("generated.html", html);
    printf("Generated:\n%.*s\n", (int) html.count, html.data);
}