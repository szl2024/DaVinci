#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graphviz.hpp>
#include <iostream>
#include <fstream>
#include <cstdlib>

using namespace boost;

// 정점의 속성 정의
struct VertexProperties {
    std::string name;
};

// 간선의 속성 정의
struct EdgeProperties {
    double weight;
};

// 그래프 타입 정의
typedef adjacency_list<
    setS,               // OutEdgeList: 간선 컨테이너 타입
    vecS,              // VertexList: 정점 컨테이너 타입
    directedS,         // Directed: 방향 그래프
    VertexProperties,  // 정점 속성
    EdgeProperties     // 간선 속성
> Graph;

typedef graph_traits<Graph>::vertex_descriptor Vertex;
typedef graph_traits<Graph>::edge_descriptor Edge;

int main() {
    // 그래프 생성
    Graph g;

    // 정점 추가
    Vertex v1 = add_vertex({"Ext"}, g);
    Vertex v2 = add_vertex({"Req1"}, g);
    Vertex v3 = add_vertex({"Req2"}, g);
    Vertex v4 = add_vertex({"Req3"}, g);

    // 간선 추가 (add_edge는 pair<edge_descriptor, bool> 반환)
    add_edge(v1, v2, {2}, g);
    add_edge(v1, v3, {1}, g);
    add_edge(v1, v4, {3}, g);
    add_edge(v2, v4, {1}, g);

    // 그래프 정보 출력
    std::cout << "Number of vertices: " << num_vertices(g) << std::endl;
    std::cout << "Number of edges: " << num_edges(g) << std::endl;

    // Graphviz DOT 파일로 저장
    std::ofstream dot_file("graph.dot");
    write_graphviz(dot_file, g,
        make_label_writer(get(&VertexProperties::name, g)),
        make_label_writer(get(&EdgeProperties::weight, g))
    );
    dot_file.close();

     // dot 명령어로 PNG 생성
    std::string cmd = "dot -Tpng graph.dot -o GraphImages/graph.png";
    int result = system(cmd.c_str());
    
    if (result == 0) {
        std::cout << "Successfully generated graph.png" << std::endl;
        
        // DOT 파일 삭제 (선택사항)
        remove("graph.dot");
    } else {
        std::cerr << "Failed to generate PNG file. "
                  << "Make sure Graphviz is installed." << std::endl;
    }

    return 0;
}