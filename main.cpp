#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

using namespace std;
using filesystem::path;

path operator""_p(const char* data, std::size_t sz) {
    return path(data, data + sz);
}

// напишите эту функцию
bool Preprocess(const path& in_file, const path& out_file, const vector<path>& include_directories){

    static regex include_file(R"/(\s*#\s*include\s*"([^"]*)"\s*)/");    // #include "..." файл
    static regex include_library(R"/(\s*#\s*include\s*<([^>]*)>\s*)/"); // #include <...> библиотека
    smatch m_file;
    smatch m_lib;
    ifstream input(in_file);
    ofstream writer(out_file, ios::out | ios::app);
    string line;
    int line_num = 1;
    while (getline(input, line))
    {
        if (regex_match(line, m_file, include_file))
        {
            path include = string(m_file[1]);
            path parent_path = in_file.parent_path();
            path include_path = parent_path / include;
            if(!exists(include_path)){
                bool flag = false;
                for(auto p: include_directories){
                    path lib_path = p / include;
                    if(exists(lib_path)){ 
                        flag = true;
                        Preprocess(lib_path, out_file, include_directories);
                    }
                }
                if(!flag){
                    cout << "unknown include file " << m_file[1] << " at file " <<  in_file.string() << " at line " << line_num << endl;
                    return false;
                }
            }
            else{
               Preprocess(include_path, out_file, include_directories);  
            }

            
        }
        else if (regex_match(line, m_lib, include_library))
        {
            path include = string(m_lib[1]);
            path parent_path = in_file.parent_path();
            path include_path = parent_path / include;
            //cout << include_path << endl;
            //Preprocess(include_path, out_file, include_directories);
            if(!exists(include_path)){
                bool flag = false;
                for(auto p: include_directories){
                    path lib_path = p / include;
                    if(exists(lib_path)){ 
                        flag = true;
                        Preprocess(lib_path, out_file, include_directories);
                    }
                }
                if(!flag) {
                    cout << "unknown include file " << m_lib[1] << " at file " <<  in_file.string() << " at line " << line_num << endl;
                    return false;
                    };
            }
        }
        else{
            writer << line << endl;
        }
        ++line_num;
    }
    

    return true;
}

string GetFileContents(string file) {
    ifstream stream(file);

    // конструируем string по двум итераторам
    return {(istreambuf_iterator<char>(stream)), istreambuf_iterator<char>()};
}

void Test() {
    error_code err;
    filesystem::remove_all("sources"_p, err);
    filesystem::create_directories("sources"_p / "include2"_p / "lib"_p, err);
    filesystem::create_directories("sources"_p / "include1"_p, err);
    filesystem::create_directories("sources"_p / "dir1"_p / "subdir"_p, err);

    {
        ofstream file("sources/a.cpp");
        file << "// this comment before include\n"
                "#include \"dir1/b.h\"\n"
                "// text between b.h and c.h\n"
                "#include \"dir1/d.h\"\n"
                "\n"
                "int SayHello() {\n"
                "    cout << \"hello, world!\" << endl;\n"
                "#   include<dummy.txt>\n"
                "}\n"s;
    }
    {
        ofstream file("sources/dir1/b.h");
        file << "// text from b.h before include\n"
                "#include \"subdir/c.h\"\n"
                "// text from b.h after include"s;
    }
    {
        ofstream file("sources/dir1/subdir/c.h");
        file << "// text from c.h before include\n"
                "#include <std1.h>\n"
                "// text from c.h after include\n"s;
    }
    {
        ofstream file("sources/dir1/d.h");
        file << "// text from d.h before include\n"
                "#include \"lib/std2.h\"\n"
                "// text from d.h after include\n"s;
    }
    {
        ofstream file("sources/include1/std1.h");
        file << "// std1\n"s;
    }
    {
        ofstream file("sources/include2/lib/std2.h");
        file << "// std2\n"s;
    }

    assert((!Preprocess("sources"_p / "a.cpp"_p, "sources"_p / "a.in"_p,
                                  {"sources"_p / "include1"_p,"sources"_p / "include2"_p})));

    ostringstream test_out;
    test_out << "// this comment before include\n"
                "// text from b.h before include\n"
                "// text from c.h before include\n"
                "// std1\n"
                "// text from c.h after include\n"
                "// text from b.h after include\n"
                "// text between b.h and c.h\n"
                "// text from d.h before include\n"
                "// std2\n"
                "// text from d.h after include\n"
                "\n"
                "int SayHello() {\n"
                "    cout << \"hello, world!\" << endl;\n"s;

    assert(GetFileContents("sources/a.in"s) == test_out.str());
}

int main() {
    Test();
}
