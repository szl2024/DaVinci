#include "../inc/main.h"
#include "../inc/graph.h"

#include "spdlog/spdlog.h"
#include "spdlog/fmt/ranges.h"

#include <locale>
#include <codecvt>
#include <sstream>
#include <iostream>
#include <memory>
#include <map>
#include <fstream>
#include <string>
#include <sys/stat.h> // for stat, mkdir
#include <unistd.h>  // for unlink
#include <cstring>  // for std::strcpy
#include <dirent.h>  // for mkdir
#include <cerrno>   // for errno

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

using namespace std;
using boost::property_tree::ptree;

struct BlockInfo {
    string BlockType;
    string Name;
    string SID;
};

struct Link {
    string In_SID;
    string Out_SID;
};

int drawGraph(const vector<BlockInfo>& blocks, const vector<Link>& links);
bool copyFile(const string& srcPath, const string& dstPath);
bool unzipFile(const string& zipPath, const string& extractDir);
bool createDirectory(const string& dirPath);
bool processFile(const string& fileName);
bool printFileContent(const string& filePath);
bool parseSystem1XML(const string& filePath, vector<BlockInfo>& blocks, vector<Link>& links);

int main()
{
    // spdlog::set_level(spdlog::level::debug);

    // 提示用户输入文件名（包括后缀名）
    //string fileName = "untitled.slx";
    cout << "请输入要选择的 SLX 文件名（包括后缀名）: ";
    string fileName;
    getline(cin, fileName);

    // 创建文件流对象
    ofstream outputFile("output.txt");
    if (!outputFile) {
        spdlog::error("无法创建文件: output.txt");
        return 1;
    }

    // 处理文件
    if (processFile(fileName)) {
        outputFile << "文件处理完成\n";

        // 输出特定文件内容
        string specificFilePath = fileName.substr(0, fileName.find_last_of('.')) + "/simulink/systems/system_1.xml";
        vector<BlockInfo> blocks;
        vector<Link> links;
        if (parseSystem1XML(specificFilePath, blocks, links)) {
            for (const auto& block : blocks) {
                outputFile << "BlockType: " << block.BlockType << ", Name: " << block.Name << ", SID: " << block.SID << "\n";
            }
            for (const auto& link : links) {
                outputFile << "In_SID: " << link.In_SID << ", Out_SID: " << link.Out_SID << "\n";
            }

            // 绘制图形
            drawGraph(blocks, links);
        } else {
            outputFile << "无法解析文件内容: " << specificFilePath << "\n";
        }
    } else {
        outputFile << "文件处理失败\n";
    }

    // 关闭文件流
    outputFile.close();

    return 0;
}

int drawGraph(const vector<BlockInfo>& blocks, const vector<Link>& links)
{
    // 创建一个映射，将 SID 映射到 BlockInfo
    map<string, const BlockInfo*> sidToBlock;
    for (const auto& block : blocks) {
        sidToBlock[block.SID] = &block;
    }

    // 创建一个映射，将 BlockType 映射到节点列表
    map<string, vector<string>> blockTypeToNodes;
    for (const auto& block : blocks) {
        blockTypeToNodes[block.BlockType].push_back(block.Name);
    }

    // 创建 graph.dot 文件
    std::ofstream dot_file("graph.dot");
    if (!dot_file) {
        spdlog::error("无法创建文件: graph.dot");
        return 1;
    }

    dot_file << "digraph G {\n";
    dot_file << "    rankdir=LR; // 横向排列\n";

    // 添加子图，将相同 BlockType 的节点排列在同一列，但不显示边框
    for (auto it = blockTypeToNodes.begin(); it != blockTypeToNodes.end(); ++it) {
        const string& blockType = it->first;
        const vector<string>& nodes = it->second;
        dot_file << "    subgraph cluster_" << blockType << " {\n";
        dot_file << "        label=\"" << blockType << "\";\n";
        dot_file << "        rank=same;\n"; // 确保同一列
        dot_file << "        style=invis;\n"; // 不显示边框
        for (const auto& node : nodes) {
            dot_file << "        \"" << node << "\" [label=\"" << node << "\\n(" << blockType << ")\"];\n";
        }
        dot_file << "    }\n";
    }

    // 添加边
    for (const auto& link : links) {
        auto inBlock = sidToBlock.find(link.In_SID);
        auto outBlock = sidToBlock.find(link.Out_SID);
        if (inBlock != sidToBlock.end() && outBlock != sidToBlock.end()) {
            dot_file << "    \"" << inBlock->second->Name << "\" -> \"" << outBlock->second->Name << "\";\n";
        } else {
            spdlog::error("无法找到 SID: {} 或 {}", link.In_SID, link.Out_SID);
        }
    }

    dot_file << "}\n";
    dot_file.close();

    // 使用 Graphviz 生成 SVG 图像
    std::string cmd = "dot -Tsvg graph.dot -o graph.svg";
    int result = std::system(cmd.c_str());
    if (result != 0) {
        spdlog::error("生成 SVG 图像失败");
        return 1;
    }

    spdlog::info("SVG 图像已生成: graph.svg");
    return 0;
}

bool copyFile(const string& srcPath, const string& dstPath)
{
    ifstream src(srcPath, ios::binary);
    if (!src) {
        spdlog::error("无法打开文件: {}", srcPath);
        return false;
    }

    ofstream dst(dstPath, ios::binary);
    if (!dst) {
        spdlog::error("无法创建文件: {}", dstPath);
        return false;
    }

    dst << src.rdbuf();

    src.close();
    dst.close();

    return true;
}

bool unzipFile(const string& zipPath, const string& extractDir)
{
    string unzipCmd = "unzip -q " + zipPath + " -d " + extractDir;
    int unzipResult = system(unzipCmd.c_str());
    if (unzipResult != 0) {
        spdlog::error("解压文件失败: {}", zipPath);
        return false;
    }

    return true;
}

bool createDirectory(const string& dirPath)
{
    struct stat st;
    if (stat(dirPath.c_str(), &st) == 0) {
        // 目录已存在
        if (S_ISDIR(st.st_mode)) {
            spdlog::info("目录已存在: {}", dirPath);
            return true;
        } else {
            spdlog::error("路径已存在但不是目录: {}", dirPath);
            return false;
        }
    }

    // 尝试创建目录
    if (mkdir(dirPath.c_str(), 0777) != 0) {
        spdlog::error("无法创建目录: {} (错误代码: {})", dirPath, errno);
        return false;
    }

    return true;
}

bool processFile(const string& fileName)
{
    // 构建文件路径
    string filePath = fileName;
    string baseName = fileName.substr(0, fileName.find_last_of('.'));
    string newFilePath = baseName + ".zip";
    string extractDir = baseName;

    // 检查文件是否存在
    struct stat buffer;
    if (stat(filePath.c_str(), &buffer) == 0) {
        // 复制文件
        if (copyFile(filePath, newFilePath)) {
            spdlog::info("文件已成功复制并重命名为: {}", newFilePath);

            // 创建解压目录
            if (createDirectory(extractDir)) {
                // 解压 ZIP 文件到指定目录
                if (unzipFile(newFilePath, extractDir)) {
                    spdlog::info("文件已成功解压到目录: {}", extractDir);
                    return true;
                } else {
                    spdlog::error("解压文件失败: {}", newFilePath);
                }
            } else {
                spdlog::error("无法创建目录: {}", extractDir);
            }
        } else {
            spdlog::error("无法复制文件: {}", filePath);
        }
    } else {
        spdlog::error("文件不存在: {}", filePath);
    }

    return false;
}

bool printFileContent(const string& filePath)
{
    ifstream file(filePath);
    if (!file) {
        spdlog::error("无法打开文件: {}", filePath);
        return false;
    }

    string line;
    while (getline(file, line)) {
        spdlog::info("{}", line);
    }

    file.close();
    return true;
}

bool parseSystem1XML(const string& filePath, vector<BlockInfo>& blocks, vector<Link>& links)
{
    try {
        ptree pt;
        read_xml(filePath, pt);

        // 解析 Block 节点
        for (const auto& blockNode : pt.get_child("System")) {
            if (blockNode.first == "Block") {
                BlockInfo block;
                block.BlockType = blockNode.second.get<string>("<xmlattr>.BlockType");
                block.Name = blockNode.second.get<string>("<xmlattr>.Name");
                block.SID = blockNode.second.get<string>("<xmlattr>.SID");
                blocks.push_back(block);
            }
        }

        // 解析 Line 节点
        for (const auto& lineNode : pt.get_child("System")) {
            if (lineNode.first == "Line") {
                Link link;
                for (const auto& pNode : lineNode.second) {
                    if (pNode.first == "P") {
                        string name = pNode.second.get<string>("<xmlattr>.Name");
                        if (name == "Src") {
                            string srcValue = pNode.second.data();
                            size_t start = srcValue.find('[') + 1;
                            size_t end = srcValue.find(',');
                            string sid = srcValue.substr(start, end - start);
                            // 提取第一个数字
                            size_t firstDigitEnd = sid.find('#');
                            if (firstDigitEnd != string::npos) {
                                link.In_SID = sid.substr(0, firstDigitEnd);
                            } else {
                                link.In_SID = sid;
                            }
                        } else if (name == "Dst") {
                            string dstValue = pNode.second.data();
                            size_t start = dstValue.find('[') + 1;
                            size_t end = dstValue.find(',');
                            string sid = dstValue.substr(start, end - start);
                            // 提取第一个数字
                            size_t firstDigitEnd = sid.find('#');
                            if (firstDigitEnd != string::npos) {
                                link.Out_SID = sid.substr(0, firstDigitEnd);
                            } else {
                                link.Out_SID = sid;
                            }
                        }
                    }
                }
                if (!link.In_SID.empty() && !link.Out_SID.empty()) {
                    links.push_back(link);
                }
            }
        }
    } catch (const boost::property_tree::ptree_error& e) {
        spdlog::error("解析 XML 文件失败: {} (错误: {})", filePath, e.what());
        return false;
    }

    return true;
}