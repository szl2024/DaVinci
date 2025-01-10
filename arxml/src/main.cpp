#include "../inc/main.h"
#include "../inc/graph.h"
#include <boost/algorithm/string.hpp>
#include "spdlog/spdlog.h"
#include "spdlog/fmt/ranges.h"
#include "boost/property_tree/ptree.hpp"
#include "boost/property_tree/xml_parser.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <boost/graph/graphviz.hpp>
#include <map>
#include <set>

using namespace std;
using namespace boost;
using namespace boost::property_tree;

// 定义 PORT 结构体
struct PORT {
    string PORT_NAME;
    string CS_SR;
    string IN_OUT; // 根据父节点设置
};

// 定义 SWC 结构体
struct SWC {
    string SWC_NAME;
    vector<PORT> ports;
};

// 读取 ARXML 文件并返回解析后的 ptree 对象
ptree readArxmlFile(const std::string& filename) {
    ptree pt;
    try {
        std::ifstream file(filename);
        if (!file) {
            throw std::runtime_error("文件不存在或无法打开: " + filename);
        }
        // 使用 xml_parser::no_comments 和 xml_parser::trim_whitespace 选项
        read_xml(file, pt, xml_parser::no_comments | xml_parser::trim_whitespace);
    } catch (const std::exception& e) {
        spdlog::error("错误: {}", e.what());
        throw;
    }
    return pt;
}

// 获取命名空间前缀
string getNamespacePrefix(const ptree& pt) {
    string nsPrefix = "";
    if (pt.count("<xmlattr>")) {
        for (const auto& attr : pt.get_child("<xmlattr>")) {
            if (attr.first == "xmlns") {
                nsPrefix = attr.first + ":";
                break;
            }
        }
    }
    return nsPrefix;
}

// 递归遍历 ptree 并提取 SWC 信息
void traverseAndExtractSWC(const ptree& pt, const string& nsPrefix, vector<SWC>& swcs) {
    for (const auto& node : pt) {
        if (node.first == nsPrefix + "APPLICATION-SW-COMPONENT-TYPE") {
            SWC swc;
            try {
                swc.SWC_NAME = node.second.get<std::string>(nsPrefix + "SHORT-NAME");
            } catch (const std::exception& e) {
                spdlog::error("获取 SHORT-NAME 时发生错误: {}", e.what());
                continue;
            }

            // 提取 PORTS 信息
            if (node.second.count(nsPrefix + "PORTS")) {
                for (const auto& portNode : node.second.get_child(nsPrefix + "PORTS")) {
                    if (portNode.first == nsPrefix + "P-PORT-PROTOTYPE" || portNode.first == nsPrefix + "R-PORT-PROTOTYPE") {
                        PORT port;
                        try {
                            port.PORT_NAME = portNode.second.get<std::string>(nsPrefix + "SHORT-NAME");
                            // 初始化 CS_SR
                            port.CS_SR = "";

                            // 检查 REQUIRED-INTERFACE-TREF 节点
                            if (portNode.second.count(nsPrefix + "REQUIRED-INTERFACE-TREF")) {
                                for (const auto& requiredInterfaceNode : portNode.second.get_child(nsPrefix + "REQUIRED-INTERFACE-TREF")) {
                                    if (requiredInterfaceNode.first == "<xmlattr>") {
                                        for (const auto& attr : requiredInterfaceNode.second) {
                                            if (attr.first == "DEST") {
                                                if (attr.second.get_value<string>() == "SENDER-RECEIVER-INTERFACE") {
                                                    port.CS_SR = "S/R";
                                                } else if (attr.second.get_value<string>() == "CLIENT-SERVER-INTERFACE") {
                                                    port.CS_SR = "C/S";
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            // 检查 PROVIDED-INTERFACE-TREF 节点 (仅适用于 P-PORT-PROTOTYPE)
                            if (portNode.first == nsPrefix + "P-PORT-PROTOTYPE" && portNode.second.count(nsPrefix + "PROVIDED-INTERFACE-TREF")) {
                                for (const auto& providedInterfaceNode : portNode.second.get_child(nsPrefix + "PROVIDED-INTERFACE-TREF")) {
                                    if (providedInterfaceNode.first == "<xmlattr>") {
                                        for (const auto& attr : providedInterfaceNode.second) {
                                            if (attr.first == "DEST") {
                                                if (attr.second.get_value<string>() == "SENDER-RECEIVER-INTERFACE") {
                                                    port.CS_SR = "S/R";
                                                } else if (attr.second.get_value<string>() == "CLIENT-SERVER-INTERFACE") {
                                                    port.CS_SR = "C/S";
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            // 根据父节点名称设置 IN_OUT 成员
                            if (portNode.first == nsPrefix + "P-PORT-PROTOTYPE") {
                                port.IN_OUT = "OUT";
                            } else if (portNode.first == nsPrefix + "R-PORT-PROTOTYPE") {
                                port.IN_OUT = "IN";
                            }
                        } catch (const std::exception& e) {
                            spdlog::error("获取 PORT 详细信息时发生错误: {}", e.what());
                            continue;
                        }
                        swc.ports.push_back(port);
                    }
                }
            }

            swcs.push_back(swc);
        }
        // 递归遍历子节点
        traverseAndExtractSWC(node.second, nsPrefix, swcs);
    }
}

// 提取并存储 APPLICATION-SW-COMPONENT-TYPE 节点下的 SHORT-NAME 节点内容
void extractAndStoreSWC(const ptree& pt, std::vector<SWC>& swcs) {
    try {
        // 获取命名空间前缀
        std::string nsPrefix = getNamespacePrefix(pt);

        // 递归遍历整个 ptree
        traverseAndExtractSWC(pt, nsPrefix, swcs);

        // 在控制台上输出 SWC 信息
        for (const auto& swc : swcs) {
            std::cout << "SWC {" << std::endl;
            std::cout << "  SWC_NAME: " << swc.SWC_NAME << std::endl;
            std::cout << "  PORTS: [" << std::endl;
            for (const auto& port : swc.ports) {
                std::cout << "    PORT {" << std::endl;
                std::cout << "      PORT_NAME: " << port.PORT_NAME << std::endl;
                std::cout << "      CS_SR: " << port.CS_SR << std::endl;
                std::cout << "      IN_OUT: " << port.IN_OUT << std::endl; // 输出 IN_OUT 成员
                std::cout << "    }" << std::endl;
            }
            std::cout << "  ]" << std::endl;
            std::cout << "}" << std::endl;
        }

        // 将 SWC 信息写入文件
        std::ofstream outputFile("output.txt");
        if (!outputFile) {
            spdlog::error("无法打开 output.txt 文件进行写入");
            return;
        }

        for (const auto& swc : swcs) {
            outputFile << "SWC {" << std::endl;
            outputFile << "  SWC_NAME: " << swc.SWC_NAME << std::endl;
            outputFile << "  PORTS: [" << std::endl;
            for (const auto& port : swc.ports) {
                outputFile << "    PORT {" << std::endl;
                outputFile << "      PORT_NAME: " << port.PORT_NAME << std::endl;
                outputFile << "      CS_SR: " << port.CS_SR << std::endl;
                outputFile << "      IN_OUT: " << port.IN_OUT << std::endl; // 输出 IN_OUT 成员
                outputFile << "    }" << std::endl;
            }
            outputFile << "  ]" << std::endl;
            outputFile << "}" << std::endl;
        }

        outputFile.close();
    } catch (const std::exception& e) {
        spdlog::error("解析 ARXML 时发生错误: {}", e.what());
    }
}

int drawGraph(const std::vector<SWC>& swcs) {
    Graph g;

    // 添加顶点
    std::map<std::string, Vertex> vertexMap;
    for (const auto& swc : swcs) {
        Vertex v = add_vertex({swc.SWC_NAME}, g);
        vertexMap[swc.SWC_NAME] = v;
    }

    // 添加边并检查重复输入端口
    std::map<std::pair<std::string, std::string>, std::map<std::string, int>> edgeMap; // {sourceSWC, targetSWC} -> {CS_SR_type -> count}
    for (const auto& swc : swcs) {
        for (const auto& port : swc.ports) {
            if (port.IN_OUT == "OUT") {
                // 查找目标 SWC
                for (const auto& targetSWC : swcs) {
                    for (const auto& targetPort : targetSWC.ports) {
                        if (targetPort.IN_OUT == "IN" && targetPort.PORT_NAME == port.PORT_NAME) {
                            // 更新边的 CS_SR 类型计数
                            edgeMap[{swc.SWC_NAME, targetSWC.SWC_NAME}][port.CS_SR]++;

                            // 如果是重复输入端口，则警告
                            if (edgeMap[{swc.SWC_NAME, targetSWC.SWC_NAME}][port.CS_SR] > 1) {
                                spdlog::warn("检测到重复输入端口: {} 的 {} 端口已从 {} 接收输入", targetSWC.SWC_NAME, targetPort.PORT_NAME, swc.SWC_NAME);
                            }
                        }
                    }
                }
            }
        }
    }

    // 添加边到图中
    for (const auto& edgePair : edgeMap) {
        const std::string& sourceSWC = edgePair.first.first;
        const std::string& targetSWC = edgePair.first.second;
        const auto& csSrCounts = edgePair.second;

        std::string edgeLabel;
        for (const auto& csSrPair : csSrCounts) {
            const std::string& csSrType = csSrPair.first;
            int count = csSrPair.second;
            if (!edgeLabel.empty()) {
                edgeLabel += ", ";
            }
            edgeLabel += fmt::format("{}: {}", csSrType, count); // 修改为 CS_SR: count 的形式
        }

        EdgeProperties edgeProps{1.0, edgeLabel}; // weight 是 double 类型，因此设置为 1.0
        add_edge(vertexMap[sourceSWC], vertexMap[targetSWC], edgeProps, g);
    }

    std::ofstream dot_file("graph.dot");
    write_graphviz(dot_file, g,
        make_label_writer(get(&VertexProperties::name, g)),
        make_label_writer(get(&EdgeProperties::label, g))
    );
    dot_file.close();

    std::string cmd = "dot -Tsvg graph.dot -o graph.svg";
    return system(cmd.c_str());
}
int main() {
    
    //std::string arxmlFilename = "OSDC_Test_Bad_swc.arxml"; // 直接指定文件名
    std::string arxmlFilename;
    std::cout << "请输入 ARXML 文件名: ";
    std::cin >> arxmlFilename;

    try {
        ptree pt = readArxmlFile(arxmlFilename);
        //printXmlContent(pt);  // 打印 ARXML 文件内容
        std::vector<SWC> swcs;
        extractAndStoreSWC(pt, swcs);  // 提取并存储 APPLICATION-SW-COMPONENT-TYPE 节点下的 SHORT-NAME 节点内容
        drawGraph(swcs); // 将 swcs 向量传递给 drawGraph 函数
    } catch (const std::exception& e) {
        spdlog::error("错误: {}", e.what());
        return 1;
    }
    return 0;
}