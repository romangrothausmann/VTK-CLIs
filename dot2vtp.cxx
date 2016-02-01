////program to convert a graph stored in a DOT-file (graphviz) in a VTP
//01: based on template.cxx



#include <boost/graph/graphviz.hpp>
#include <iostream>

#include <vtkSmartPointer.h>
#include <vtkMutableUndirectedGraph.h>
#include <vtkGraphToPolyData.h>
#include <vtkXMLPolyDataWriter.h>

#include <vtkCallbackCommand.h>
#include <vtkCommand.h>


#define VTK_CREATE(type, name) vtkSmartPointer<type> name = vtkSmartPointer<type>::New()


void FilterEventHandlerVTK(vtkObject* caller, long unsigned int eventId, void* clientData, void* callData){

    vtkAlgorithm *filter= static_cast<vtkAlgorithm*>(caller);

    switch(eventId){
    case vtkCommand::ProgressEvent:
        fprintf(stderr, "\r%s progress: %5.1f%%", filter->GetClassName(), 100.0 * filter->GetProgress());//stderr is flushed directly
        break;
    case vtkCommand::EndEvent:
        std::cerr << std::endl << std::flush;
        break;

        //// VTK does not throw errors (http://public.kitware.com/pipermail/vtkusers/2009-February/050805.html) use Error-Events: http://www.cmake.org/Wiki/VTK/Examples/Cxx/Utilities/ObserveError
    case vtkCommand::ErrorEvent:
        std::cerr << "Error: " << static_cast<char*>(callData) << std::endl << std::flush;
        break;
    case vtkCommand::WarningEvent:
        std::cerr << "Warning: " << static_cast<char*>(callData) << std::endl << std::flush;
        break;
        }
    }

// http://stackoverflow.com/questions/29898195/boostread-graphviz-how-to-read-out-properties
// http://stackoverflow.com/questions/29496182/read-graphviz-in-boostgraph-pass-to-constructor/29501850#29501850
// http://stackoverflow.com/questions/20847295/how-to-read-in-pos-attribute-of-dot-file-using-boost
struct DotVertex { 
    std::string name;
    std::string label;
    std::vector<double> pos;
    };

struct DotEdge {
    std::string label;
    };

// http://lists.boost.org/boost-users/2010/06/59920.php
inline std::istream& operator>> (std::istream& ss, std::vector<double>& vect){ 
    double i;
    while(ss >> i){
	vect.push_back(i);
	if(ss.peek() == ',')
	    ss.ignore();
	}

    return ss; 
    }

inline std::ostream& operator<< (std::ostream& out, const std::vector<double>& vect) {
    size_t last = vect.size() - 1;
    for (size_t i = 0; i < vect.size(); ++i) {
        out << vect[i];
        if (i != last)
            out << ",";
	}

    return out;
    }


int main (int argc, char *argv[]){

    if (argc != 4){
        std::cerr << "Usage: " << argv[0]
                  << " input"
                  << " output"
                  << " compress"
                  << std::endl;
        return EXIT_FAILURE;
        }

    if(!(strcasestr(argv[1],".dot") || strcasestr(argv[1],".gv"))) {
        std::cerr << "The input should end with .dot or .gv" << std::endl;
        return -1;
        }

    if(!(strcasestr(argv[2],".vtp"))) {
        std::cerr << "The output should end with .vtp" << std::endl;
        return -1;
        }


    VTK_CREATE(vtkCallbackCommand, eventCallbackVTK);
    eventCallbackVTK->SetCallback(FilterEventHandlerVTK);

    typedef boost::adjacency_list < boost::vecS, boost::vecS, boost::undirectedS, DotVertex, DotEdge> DotGraphT;
    DotGraphT dotGraph;

    boost::dynamic_properties dp(boost::ignore_other_properties);

    dp.property("node_id",     boost::get(&DotVertex::name,        dotGraph));
    dp.property("label",       boost::get(&DotVertex::label,       dotGraph));
    dp.property("pos",         boost::get(&DotVertex::pos,         dotGraph));

    dp.property("label",       boost::get(&DotEdge::label,         dotGraph));

    std::ifstream fn(argv[1]);
    boost::read_graphviz(fn, dotGraph, dp);
   

    vtkSmartPointer<vtkMutableUndirectedGraph> ograph= vtkSmartPointer<vtkMutableUndirectedGraph>::New();
    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();

    typedef boost::graph_traits<DotGraphT>::vertex_iterator vertex_iter;
    std::pair<vertex_iter, vertex_iter> vp;
    for(vp = vertices(dotGraph); vp.first != vp.second; ++vp.first){
        //std::cout << dotGraph[*vp.first].pos << std::endl;
        ograph->AddVertex();
    	points->InsertNextPoint(dotGraph[*vp.first].pos[0], dotGraph[*vp.first].pos[1], 0);
        }

    std::pair<DotGraphT::edge_iterator, DotGraphT::edge_iterator> edgeIteratorRange = boost::edges(dotGraph);
    for(DotGraphT::edge_iterator edgeIterator = edgeIteratorRange.first; edgeIterator != edgeIteratorRange.second; ++edgeIterator){
        ograph->AddEdge(boost::source(*edgeIterator, dotGraph), boost::target(*edgeIterator, dotGraph));
        }

    ograph->SetPoints(points);


    VTK_CREATE(vtkGraphToPolyData, filter);
    filter->SetInputData(ograph);
    filter->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
    filter->Update();

    VTK_CREATE(vtkXMLPolyDataWriter, writer);
    writer->SetInputConnection(filter->GetOutputPort());
    writer->SetFileName(argv[2]);
    writer->SetDataModeToBinary();//SetDataModeToAscii()//SetDataModeToAppended()
    if(atoi(argv[3]))
        writer->SetCompressorTypeToZLib();//default
    else
        writer->SetCompressorTypeToNone();
    writer->AddObserver(vtkCommand::AnyEvent, eventCallbackVTK);
    writer->Write();

    return EXIT_SUCCESS;
    }


