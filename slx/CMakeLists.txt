cmake_minimum_required(VERSION 3.10)
project(Requirements_Parser)
include_directories(external/spdlog_v1.15.0/include)
# ���� Boost ��Ŀ¼�������·��
set(BOOST_ROOT "D:/boost_1_87_0")  # Boost ��ѹ�ĸ�Ŀ¼
set(BOOST_INCLUDEDIR "D:/boost_1_87_0")  # Boost ��ͷ�ļ�Ŀ¼
set(BOOST_LIBRARYDIR "D:/boost_1_87_0/stage/lib")  # Boost �Ŀ��ļ�Ŀ¼��ͨ���ڱ��������

# ���� Boost ��
find_package(Boost 1.45.0 REQUIRED COMPONENTS graph)

# ���� Boost ��ͷ�ļ�
include_directories(${Boost_INCLUDE_DIRS})

# ����ҵ��� Boost ��Ϣ
message(STATUS "Boost include dir: ${Boost_INCLUDE_DIRS}")
message(STATUS "Boost libraries: ${Boost_LIBRARIES}")

# ���� C++ ��׼
set(CMAKE_CXX_STANDARD 14)


# ���� MSVC ��������־�����ʹ�� Visual Studio��
if (MSVC) 
    message(STATUS "Compiler is MSVC")
    set(CMAKE_CXX_FLAGS "/utf-8")
    set(CMAKE_C_FLAGS "/utf-8")
endif()

# ���Ҳ�����Դ�ļ�
file(GLOB SOURCES "src/*.cpp")
add_executable(rp ${SOURCES})

# ���� Boost ��
target_link_libraries(rp PRIVATE ${Boost_LIBRARIES})
