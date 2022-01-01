#include <map>
#include <set>
#include <iostream>
#include <fstream>
#include <regex>
#include <dirent.h>

using namespace std;

// 存放函数调用关系，键为函数名，值为被调用函数的vector
map<string, set<string>> func_call;
// 调用关系
const string relation_line = "-->";     /* NOLINT */
// 画图方式
// TB，从上到下
// TD，从上到下
// BT，从下到上
// RL，从右到左
// LR，从左到右
const string pic_type = "LR";     /* NOLINT */
// 排除系统函数，即只考虑自己定义的函数
bool excluded_system_function = true;
// 要检测的项目根目录
string project_dir;
// 要检测的语言类型，目前只支持.c和.cpp
string file_type;

/**
 *
 * 转换时间到字符串
 * @param raw_time 原始时间
 * @return
 */
string time_to_str(time_t raw_time) {
    tm *tm_t = localtime(&raw_time);
    stringstream ss;
    ss << tm_t->tm_year + 1900 << "-" << tm_t->tm_mon + 1 << "-" << tm_t->tm_mday
       << " " << tm_t->tm_hour << "." << tm_t->tm_min << "." << tm_t->tm_sec;

    return ss.str();
}

/**
 * 打印函数之间的调用关系
 * @return true: 成功; false: 失败
 */
bool print_result() {
    // 遍历函数调用关系
    map<string, set<string>>::iterator iter;

    fstream outfile;
    string file_name = "result " + time_to_str(time(nullptr)) + ".md";

    outfile.open(file_name, ios::app);   //每次写都定位的文件结尾，不会丢失原来的内容，用out则会丢失原来的内容
    if (!outfile.is_open()) {
        cout << "open file failed" << endl;
        return false;
    }
    outfile << "# " << project_dir << "函数调用关系" << endl;
    outfile << "```mermaid" << endl << "graph " << pic_type << endl;
    for (iter = func_call.begin(); iter != func_call.end(); iter++) {
        for (const string &func_name: iter->second) {
            if (excluded_system_function) {
                if (func_call.count(func_name) > 0) {
                    outfile << iter->first << relation_line << func_name << endl;
                }
            } else {
                outfile << iter->first << relation_line << func_name << endl;
            }

        }
    }
    if (func_call.empty()) {
        outfile << endl;
    }
    outfile << "```" << endl << endl;
    outfile.close();
    cout << "文件已保存到" << file_name << "，请使用支持markdown画图的软件查看" << endl;
    return true;
}

/**
 * 判断一个字符串是否包含函数名
 * @param target_str 待解析字符串
 * @return 包含则返回函数名，否则返回空字符串
 */
string is_function_name(const string &target_str) {
    // 匹配函数名及括号，括号前可以有空格
    regex pattern(R"(([a-zA-Z_]+\w*)\s*\()");
    smatch results;
    if (!regex_search(target_str, results, pattern)) {
        return "";
    }
    string func_name = results.begin()->str();
    func_name = func_name.substr(0, func_name.find('('));   // 去掉左括号
    func_name = func_name.substr(0, func_name.find(' '));   // 去掉空格
    return func_name;
}

/**
 * 判断一个字符串是否为函数声明
 * @param target_str 待解析字符串
 * @return true 或 false
 */
bool is_function_define(const string &target_str) {
    // 匹配函数名及括号，小括号和中括号前可以有空格
    regex pattern2(
            R"((?:char|int|float|double|void|bool|string|long|long long)\s*\*?\s*([a-zA-Z_]+\w*)\s*\(.*\)\s*\{)");
    smatch results;
    if (regex_search(target_str, results, pattern2)) {
        return true;
    }
    return false;
}

/**
 * 读取一个文件，并解析他的函数调用关系
 * @param file_path 文件的绝对路径
 */
void read_file(const string &file_path) {
    ifstream file_stream(file_path);
    if (!file_stream.is_open()) {
        cout << "open file failed" << endl;
        return;
    }
    string temp_line;
    string cur_func_name;   // 当前检测的函数名
    string temp_func_name;  // 当前匹配到的函数名

    while (getline(file_stream, temp_line)) {
        bool is_func_define = is_function_define(temp_line);
        if (is_func_define) {
            cur_func_name = is_function_name(temp_line);
        } else {
            temp_func_name = is_function_name(temp_line);
            if (!temp_func_name.empty() && !cur_func_name.empty()) {
                func_call[cur_func_name].insert(temp_func_name);
            }
        }
    }
    file_stream.close();
}

/**
 * 遍历文件夹
 * @param root_dir 根文件夹
 */
void detection_folder(const string &root_dir) {
    DIR *dir_pointer = opendir(root_dir.c_str());
    if (!dir_pointer)
        return;
    for (dirent *dp = readdir(dir_pointer); dp != nullptr; dp = readdir(dir_pointer)) {
        string file_name = dp->d_name;
        if (file_name == "." || file_name == "..") {
            continue;
        }
        size_t dot_index = file_name.find('.');
        if (dot_index >= 0 && dot_index < file_name.length()) {
            // 文件处理
            string ss = file_name.substr(file_name.length() - file_type.length(), file_type.length());
            if (ss == file_type) {
                read_file(root_dir + string("/") + dp->d_name);
            }
        } else {
            // 文件夹处理
            detection_folder(root_dir + string("/") + dp->d_name);
        }
    }
    closedir(dir_pointer);
}

/**
 * 获取参数
 */
void get_param() {
    cout << "请输入要打印函数关系的项目文件夹：" << endl;
    cin >> project_dir;
    string last_char = project_dir.substr(project_dir.length() - 1, 1);
    if (last_char == "/" || last_char == "\\") {
        project_dir = project_dir.substr(0, project_dir.length() - 1);
    }

    // 核验文件夹
    DIR *dir_pointer = opendir(project_dir.c_str());
    if (!dir_pointer) {
        cout << "无效的文件夹地址!";
        exit(1);
    }
    cout << "是否只考虑用户编写的函数？输入'y'只考虑用户编写的函数，否则会连同库函数一起分析" << endl;
    string input_str;
    cin >> input_str;
    excluded_system_function = input_str == "y";
    cout << "默认情况下针对.c文件进行分析，输入'y'可转为.cpp" << endl;
    cin >> input_str;
    file_type = input_str == "y" ? ".cpp" : ".c";
}

int main() {
    get_param();
    detection_folder(project_dir);
    print_result();
    return 0;
}
