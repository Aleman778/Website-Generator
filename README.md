# [generator.h](https://github.com/Aleman778/Website-Generator/blob/main/generator.h)
                
Is a simple API for creating static site generators, and is shipped in a single C header file that can easily be included in your project. This repository includes a very simple demo, for an example implementation see the [demo.c](https://github.com/Aleman778/website_generator/blob/main/demo.c) file. And the result from running that is the [generated.html](https://github.com/Aleman778/Website-Generator/blob/main/generated.html). NOTE: this is a very early WIP and the API will change a lot.

## Features
- Basic IO reading and writing entire file
- Markdown parsing, generated in to DOM structure
- Generating HTML from DOM structure
- Basic string template system
- More to come...

## Usage (WIP)
Main functions
```C
string read_entire_file(const char* filepath);

bool write_entire_file(const char* filepath, string contents);
    
Dom read_markdown_file(const char* filename);

string generate_html_from_dom(Dom* dom);

string template_process_string(string source, int argc, string* args);
```
  
