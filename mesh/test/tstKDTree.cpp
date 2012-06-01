//---------------------------------------------------------------------------//
/*! 
 * \file tstKDTree.cpp
 * \author Stuart R. Slattery
 * \brief kD-Tree unit tests.
 */
//---------------------------------------------------------------------------//

#include <iostream>
#include <vector>
#include <cmath>
#include <sstream>
#include <algorithm>
#include <cassert>

#include <DTK_KDTree.hpp>
#include <DTK_Mesh.hpp>
#include <DTK_DataSource.hpp>
#include <DTK_CoreTypes.hpp>
#include <DTK_NodeTraits.hpp>
#include <DTK_ElementTraits.hpp>
#include <DTK_FieldTraits.hpp>
#include <DTK_Exception.hpp>

#include <mpi.h>

#include <Teuchos_UnitTestHarness.hpp>
#include <Teuchos_DefaultComm.hpp>
#include <Teuchos_DefaultMpiComm.hpp>
#include <Teuchos_RCP.hpp>
#include <Teuchos_OpaqueWrapper.hpp>
#include <Teuchos_TypeTraits.hpp>

//---------------------------------------------------------------------------//
// MPI Setup
//---------------------------------------------------------------------------//

template<class Ordinal>
Teuchos::RCP<const Teuchos::Comm<Ordinal> > getDefaultComm()
{
#ifdef HAVE_MPI
    return Teuchos::DefaultComm<Ordinal>::getComm();
#else
    return Teuchos::rcp(new Teuchos::SerialComm<Ordinal>() );
#endif
}

//---------------------------------------------------------------------------//
// Node Implementation
//---------------------------------------------------------------------------//

class MyNode
{
  private:

    std::size_t d_handle;
    std::vector<double> d_coords;

  public:

    typedef int    handle_type;
    typedef double coordinate_type;
    
    MyNode( double x, double y, double z, int handle )
	: d_handle( handle )
    {
	d_coords.push_back(x);
	d_coords.push_back(y);
	d_coords.push_back(z);
    }

    ~MyNode()
    { /* ... */ }

    int handle() const
    { return d_handle; }

    std::vector<double>::const_iterator coordsBegin() const
    { return d_coords.begin(); }

    std::vector<double>::const_iterator coordsEnd() const
    { return d_coords.end(); }
};

//---------------------------------------------------------------------------//
// Element Implementation
//---------------------------------------------------------------------------//

class MyHex
{
  private:

    std::size_t d_handle;
    std::vector<int> d_connectivity;

  public:

    typedef std::size_t handle_type;

    MyHex( int node_0, int node_1, int node_2, int node_3,
	   int node_4, int node_5, int node_6, int node_7,
	   std::size_t handle )
	: d_handle( handle )
    {
	d_connectivity.push_back( node_0 );
	d_connectivity.push_back( node_1 );
	d_connectivity.push_back( node_2 );
	d_connectivity.push_back( node_3 );
	d_connectivity.push_back( node_4 );
	d_connectivity.push_back( node_5 );
	d_connectivity.push_back( node_6 );
	d_connectivity.push_back( node_7 );
    }

    ~MyHex()
    { /* ... */ }

    int handle() const
    { return d_handle; }

    std::vector<int>::const_iterator connectivityBegin() const
    { return d_connectivity.begin(); }

    std::vector<int>::const_iterator connectivityEnd() const
    { return d_connectivity.end(); }
};

//---------------------------------------------------------------------------//
// DTK Traits Specializations
//---------------------------------------------------------------------------//
namespace DataTransferKit
{

//---------------------------------------------------------------------------//
// NodeTraits specialization for the MyNode implementation.
template<>
struct NodeTraits<MyNode>
{
    typedef typename MyNode::handle_type                 handle_type;
    typedef typename MyNode::coordinate_type             coordinate_type;
    typedef typename std::vector<double>::const_iterator 
    const_coordinate_iterator;
    
    static inline std::size_t dim()
    { return 3;}
    
    static inline handle_type handle( const MyNode& node ) 
    { return node.handle(); }
    
    static inline const_coordinate_iterator coordsBegin( const MyNode& node ) 
    { return node.coordsBegin(); }

    static inline const_coordinate_iterator coordsEnd( const MyNode& node ) 
    { return node.coordsEnd(); }
};

//---------------------------------------------------------------------------//
// ElementTraits specialization for the MyHex implementation.
template<>
struct ElementTraits<MyHex>
{
    typedef typename MyHex::handle_type              handle_type;
    typedef typename std::vector<int>::const_iterator 
    const_connectivity_iterator;

    static inline std::size_t type()
    { return DTK_REGION; }

    static inline std::size_t topology()
    { return DTK_HEXAHEDRON; }

    static inline std::size_t numNodes()
    { return 8; }

    static inline handle_type handle( const MyHex &hex )
    { return hex.handle(); }

    static inline const_connectivity_iterator 
    connectivityBegin( const MyHex &hex )
    { return hex.connectivityBegin(); }

    static inline const_connectivity_iterator 
    connectivityEnd( const MyHex &hex )
    { return hex.connectivityEnd(); }
};

//---------------------------------------------------------------------------//
// FieldTraits specialization for the node field.
template<>
struct FieldTraits< std::vector<MyNode> >
{
    typedef MyNode                                value_type;
    typedef std::vector<MyNode>::iterator         iterator;
    typedef std::vector<MyNode>::const_iterator   const_iterator;
    
    static inline std::size_t size( const std::vector<MyNode> &node_field )
    { return node_field.size(); }

    static iterator begin( std::vector<MyNode> &node_field )
    { return node_field.begin(); }

    static const_iterator begin( const std::vector<MyNode> &node_field )
    { return node_field.begin(); }

    static inline iterator end( std::vector<MyNode> &node_field )
    { return node_field.end(); }

    static inline const_iterator end( const std::vector<MyNode> &node_field )
    { return node_field.end(); }

    static inline bool empty( const std::vector<MyNode> &node_field )
    { return node_field.empty(); }
};

//---------------------------------------------------------------------------//
// FieldTraits specialization for the element field.
template<>
struct FieldTraits< std::vector<MyHex> >
{
    typedef MyHex                                value_type;
    typedef std::vector<MyHex>::iterator         iterator;
    typedef std::vector<MyHex>::const_iterator   const_iterator;
    
    static inline std::size_t size( const std::vector<MyHex> &quad_field )
    { return quad_field.size(); }

    static inline iterator begin( std::vector<MyHex> &quad_field )
    { return quad_field.begin(); }

    static inline const_iterator begin( const std::vector<MyHex> &quad_field )
    { return quad_field.begin(); }

    static inline iterator end( std::vector<MyHex> &quad_field )
    { return quad_field.end(); }

    static inline const_iterator end( const std::vector<MyHex> &quad_field )
    { return quad_field.end(); }

    static inline bool empty(  const std::vector<MyHex> &quad_field )
    { return quad_field.empty(); }
};

//---------------------------------------------------------------------------//
// FieldTraits specialization for the data field.
template<>
struct FieldTraits< std::vector<double> >
{
    typedef MyNode value_type;
    typedef std::vector<double>::iterator iterator;
    typedef std::vector<double>::const_iterator const_iterator;
    
    static inline std::size_t size( const std::vector<double> &data_field )
    { return data_field.size(); }

    static inline iterator begin( std::vector<double> &data_field )
    { return data_field.begin(); }

    static inline const_iterator begin( const std::vector<double> &data_field )
    { return data_field.begin(); }

    static inline iterator end( std::vector<double> &data_field )
    { return data_field.end(); }

    static inline const_iterator end( const std::vector<double> &data_field )
    { return data_field.end(); }

    static inline bool empty( const std::vector<double> &data_field )
    { return data_field.empty(); }
};

} // end namespace DataTransferKit

//---------------------------------------------------------------------------//
// DataSource Implementation
//---------------------------------------------------------------------------//
class MyDataSource : public DataTransferKit::DataSource< std::vector<MyNode>,
							 std::vector<MyHex>,
							 std::vector<double> >
{
  private:

    std::vector<MyNode> d_nodes;
    std::vector<MyHex> d_elements;
    std::vector<double> d_element_data;
    MPI_Comm d_comm;

    void createMesh()
    {
	// Make some nodes.
	d_nodes.push_back( MyNode(0.0, 0.0, 0.0, 0) );
	d_nodes.push_back( MyNode(1.0, 0.0, 0.0, 4) );
	d_nodes.push_back( MyNode(1.0, 1.0, 0.0, 9) );
	d_nodes.push_back( MyNode(0.0, 1.0, 0.0, 2) );
	d_nodes.push_back( MyNode(0.0, 0.0, 1.0, 3) );
	d_nodes.push_back( MyNode(1.0, 0.0, 1.0, 8) );
	d_nodes.push_back( MyNode(1.0, 1.0, 1.0, 1) );
	d_nodes.push_back( MyNode(0.0, 1.0, 1.0, 6) );
	d_nodes.push_back( MyNode(0.0, 0.0, 2.0, 12) );
	d_nodes.push_back( MyNode(1.0, 0.0, 2.0, 7) );
	d_nodes.push_back( MyNode(1.0, 1.0, 2.0, 13) );
	d_nodes.push_back( MyNode(0.0, 1.0, 2.0, 5) );

	// Make 2 hexahedrons.
	d_elements.push_back( MyHex( 0, 4, 9, 2, 3, 8, 1, 6, 0 ) );
	d_elements.push_back( MyHex( 3, 8, 1, 6, 12, 7, 13, 5, 1 ) ); 

	// Add some data for the hexes.
	d_element_data.push_back( 1.5 );
	d_element_data.push_back( 3.5 );
    }

  public:

    MyDataSource()
    { 
	// Build the mesh.
	createMesh();

	// Get the raw MPI_Comm out of Teuchos.
	Teuchos::RCP< const Teuchos::Comm<int> > comm = getDefaultComm<int>();
	Teuchos::RCP< const Teuchos::MpiComm<int> > mpi_comm = 
	    Teuchos::rcp_dynamic_cast< const Teuchos::MpiComm<int> >( comm );
	Teuchos::RCP< const Teuchos::OpaqueWrapper<MPI_Comm> > opaque_comm = 
	    mpi_comm->getRawMpiComm();
	d_comm = (*opaque_comm)();
   }

    ~MyDataSource()
    { /* ... */ }

    const MPI_Comm& getSourceComm()
    {
	return d_comm;
    }

    bool isFieldSupported( const std::string &field_name )
    {
	bool return_val = false;
	if ( field_name == "MY_DATA_FIELD" )
	{
	    return_val = true;
	}
	return return_val;
    }

    const std::vector<MyNode>& getSourceMeshNodes()
    {
	return d_nodes;
    }

    const std::vector<MyHex>& getSourceMeshElements()
    {
	return d_elements;
    }

    const std::vector<double> evaluateFieldOnTargetNodes( 
	const std::string &field_name,
	const std::vector<MyHex::handle_type> &element_handles,
	const std::vector<MyNode::coordinate_type> &node_coordinates )
    {
	if ( field_name == "MY_DATA_FIELD" )
	{
	    return d_element_data;
	}
	else
	{
	    std::vector<double> empty_vec;
	    return empty_vec;
	}
    }
};

//---------------------------------------------------------------------------//
// Tests
//---------------------------------------------------------------------------//

// Point inclusion test.
TEUCHOS_UNIT_TEST( TopologyTools, topology_tools_test )
{
    using namespace DataTransferKit;

    // Create a DataSource
    Teuchos::RCP< DataSource< std::vector<MyNode>,
			      std::vector<MyHex>,
			      std::vector<double> > > data_source 
	= Teuchos::rcp( new MyDataSource() );

    // Create a mesh.
    Teuchos::RCP< Mesh<MyHex::handle_type> > mesh = 
	createMeshFromDataSource( data_source );

    // Create a KDTree.
    KDTree<MyHex::handle_type> kd_tree( mesh );

    // Build the tree.
    kd_tree.build();

    // Search the tree for some points we know we will find.
    double point_0[3] = { 0.5, 0.45, 0.98 };
    double point_1[3] = { 0.2, 0.9, 1.32 };
    TEST_ASSERT( kd_tree.findPoint( point_0 ) == 0 );
    TEST_ASSERT( kd_tree.findPoint( point_1 ) == 1 );

    // Test points we know we won't find. We expect a PointNotFound()
    // exception to be thrown here.
    double point_2[3] = { 2.9, -0.5, 9.5 };
    double point_3[3] = { 0.1, 1.5, -4.8 };
    bool caught_exception = false;
    try 
    {
	MyHex::handle_type does_not_exist = kd_tree.findPoint( point_2 );
    }
    catch( PointNotFound& exception )
    {
	caught_exception = true;
    }
    TEST_ASSERT( caught_exception );

    caught_exception = false;
    try 
    {
	MyHex::handle_type does_not_exist = kd_tree.findPoint( point_3 );
    }
    catch( PointNotFound& exception )
    {
	caught_exception = true;
    }
    TEST_ASSERT( caught_exception );
}

//---------------------------------------------------------------------------//
// end tstKDTree.cpp
//---------------------------------------------------------------------------//