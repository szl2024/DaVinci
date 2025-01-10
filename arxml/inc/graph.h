#ifndef GRAPH_H
#define GRAPH_H

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graphviz.hpp>
#include <string>//ddd

// 정점의 속성 정의
struct VertexProperties {
    std::string name;
};

// 간선의 속성 정의
struct EdgeProperties {
    double weight;
    std::string label;
};

// 그래프 타입 정의
typedef boost::adjacency_list<
    boost::setS,               // OutEdgeList: 간선 컨테이너 타입
    boost::vecS,              // VertexList: 정점 컨테이너 타입
    boost::directedS,         // Directed: 방향 그래프
    VertexProperties,  // 정점 속성
    EdgeProperties     // 간선 속성
> Graph;

typedef boost::graph_traits<Graph>::vertex_descriptor Vertex;
typedef boost::graph_traits<Graph>::edge_descriptor Edge;

#endif