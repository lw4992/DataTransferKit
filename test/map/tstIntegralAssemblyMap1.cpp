//---------------------------------------------------------------------------//
/*!
 * \file tstIntegralAssemblyMap1.cpp
 * \author Stuart R. Slattery
 * \brief Integral assembly map unit test 1 for signed ordinals.
 */
//---------------------------------------------------------------------------//

#include <iostream>
#include <vector>
#include <map>
#include <cmath>
#include <sstream>
#include <algorithm>
#include <cassert>
#include <ctime>
#include <cstdlib>

#include <DTK_IntegralAssemblyMap.hpp>
#include <DTK_FieldTraits.hpp>
#include <DTK_FieldIntegrator.hpp>
#include <DTK_FieldManager.hpp>
#include <DTK_MeshTypes.hpp>
#include <DTK_MeshTraits.hpp>
#include <DTK_MeshTools.hpp>
#include <DTK_MeshManager.hpp>
#include <DTK_GeometryTraits.hpp>
#include <DTK_GeometryManager.hpp>
#include <DTK_Cylinder.hpp>
#include <DTK_Box.hpp>

#include <Teuchos_UnitTestHarness.hpp>
#include <Teuchos_DefaultComm.hpp>
#include <Teuchos_CommHelpers.hpp>
#include <Teuchos_RCP.hpp>
#include <Teuchos_ArrayRCP.hpp>
#include <Teuchos_Ptr.hpp>
#include <Teuchos_OpaqueWrapper.hpp>
#include <Teuchos_Array.hpp>
#include <Teuchos_ArrayView.hpp>
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
// Field implementation.
//---------------------------------------------------------------------------//
class MyField
{
  public:

    typedef double value_type;
    typedef Teuchos::Array<double>::size_type size_type;
    typedef Teuchos::Array<double>::iterator iterator;
    typedef Teuchos::Array<double>::const_iterator const_iterator;

    MyField( size_type size, int dim )
	: d_dim( dim )
	, d_data( dim*size, 0.0 )
    { /* ... */ }

    ~MyField()
    { /* ... */ }

    int dim() const
    { return d_dim; }

    size_type size() const
    { return d_data.size(); }

    bool empty() const
    { return d_data.empty(); }

    iterator begin()
    { return d_data.begin(); }

    const_iterator begin() const
    { return d_data.begin(); }

    iterator end()
    { return d_data.end(); }

    const_iterator end() const
    { return d_data.end(); }

    Teuchos::Array<double>& getData()
    { return d_data; }

    const Teuchos::Array<double>& getData() const
    { return d_data; }

  private:
    int d_dim;
    Teuchos::Array<double> d_data;
};

//---------------------------------------------------------------------------//
// DTK implementations.
//---------------------------------------------------------------------------//
namespace DataTransferKit
{

//---------------------------------------------------------------------------//
// Field Traits specification for MyField
template<>
class FieldTraits<MyField>
{
  public:

    typedef MyField                    field_type;
    typedef double                     value_type;
    typedef MyField::size_type         size_type;
    typedef MyField::iterator          iterator;
    typedef MyField::const_iterator    const_iterator;

    static inline size_type dim( const MyField& field )
    { return field.dim(); }

    static inline size_type size( const MyField& field )
    { return field.size(); }

    static inline bool empty( const MyField& field )
    { return field.empty(); }

    static inline iterator begin( MyField& field )
    { return field.begin(); }

    static inline const_iterator begin( const MyField& field )
    { return field.begin(); }

    static inline iterator end( MyField& field )
    { return field.end(); }

    static inline const_iterator end( const MyField& field )
    { return field.end(); }
};

} // end namespace DataTransferKit

//---------------------------------------------------------------------------//
// FieldIntegrator Implementation.
class MyIntegrator : public DataTransferKit::FieldIntegrator<
    DataTransferKit::MeshContainer<int> ,MyField>
{
  public:

    MyIntegrator( const DataTransferKit::MeshContainer<int>& mesh, 
		 const Teuchos::RCP< const Teuchos::Comm<int> >& comm )
	: d_mesh( mesh )
	, d_comm( comm )
    { /* ... */ }

    ~MyIntegrator()
    { /* ... */ }

    // If the global id is valid, then set the element integral to 2.0
    MyField integrate( 
	const Teuchos::ArrayRCP<
	    DataTransferKit::MeshContainer<int>::global_ordinal_type>& elements )
    {
	int num_elements = elements.size();
	MyField integrated_data( num_elements, 3 );
	for ( int n = 0; n < elements.size(); ++n )
	{
	    if ( std::find( d_mesh.elementsBegin(),
			    d_mesh.elementsEnd(),
			    elements[n] ) != d_mesh.elementsEnd() )
	    {
		*(integrated_data.begin() + n ) = 2.0;
		*(integrated_data.begin() + n + num_elements) = 2.0;
		*(integrated_data.begin() + n + 2*num_elements) = 2.0;
	    }
	    else
	    {
 		*(integrated_data.begin() + n ) = 6789.443;
		*(integrated_data.begin() + n + num_elements) = 6789.443;
		*(integrated_data.begin() + n + 2*num_elements) = 6789.443;
	    }
	}
	return integrated_data;
    }

  private:

    DataTransferKit::MeshContainer<int>  d_mesh;
    Teuchos::RCP< const Teuchos::Comm<int> > d_comm;
};

//---------------------------------------------------------------------------//
// ElementMeasure Implementation.
class MyMeasure : public DataTransferKit::ElementMeasure<
    DataTransferKit::MeshContainer<int> >
{
  public:

    MyMeasure( const DataTransferKit::MeshContainer<int>& mesh, 
		 const Teuchos::RCP< const Teuchos::Comm<int> >& comm )
	: d_mesh( mesh )
	, d_comm( comm )
    { /* ... */ }

    ~MyMeasure()
    { /* ... */ }

    // If the global id is valid, then set the element measure to 1, -1 if
    // invalid.
    Teuchos::Array<double> measure( 
	const Teuchos::ArrayRCP<
	    DataTransferKit::MeshContainer<int>::global_ordinal_type>& elements )
    {
	Teuchos::Array<double> measures( elements.size() );
	for ( int n = 0; n < elements.size(); ++n )
	{
	    if ( std::find( d_mesh.elementsBegin(),
			    d_mesh.elementsEnd(),
			    elements[n] ) != d_mesh.elementsEnd() )
	    {
		measures[n] = 1.0;
	    }
	    else
	    {
 		measures[n] = -1.0;
	    }
	}
	return measures;
    }

  private:

    DataTransferKit::MeshContainer<int>  d_mesh;
    Teuchos::RCP< const Teuchos::Comm<int> > d_comm;
};

//---------------------------------------------------------------------------//
// Mesh create functions.
//---------------------------------------------------------------------------//
Teuchos::RCP<DataTransferKit::MeshContainer<int> > 
buildTetMesh( int my_rank, int my_size, int edge_length, int elem_offset )
{
    // Make some vertices.
    int num_vertices = edge_length*edge_length*2;
    int vertex_dim = 3;
    Teuchos::ArrayRCP<int> vertex_handles( num_vertices );
    Teuchos::ArrayRCP<double> coords( vertex_dim*num_vertices );
    int idx;
    for ( int j = 0; j < edge_length; ++j )
    {
	for ( int i = 0; i < edge_length; ++i )
	{
	    idx = i + j*edge_length;
	    vertex_handles[ idx ] = (int) num_vertices*my_rank + idx + elem_offset;
	    coords[ idx ] = i;
	    coords[ num_vertices + idx ] = j;
	    coords[ 2*num_vertices + idx ] = my_rank;
	}
    }
    for ( int j = 0; j < edge_length; ++j )
    {
	for ( int i = 0; i < edge_length; ++i )
	{
	    idx = i + j*edge_length + num_vertices / 2;
	    vertex_handles[ idx ] = (int) num_vertices*my_rank + idx + elem_offset;
	    coords[ idx ] = i;
	    coords[ num_vertices + idx ] = j;
	    coords[ 2*num_vertices + idx ] = my_rank+1;
	}
    }
    
    // Make the tetrahedrons. 
    int num_elements = (edge_length-1)*(edge_length-1)*5;
    Teuchos::ArrayRCP<int> tet_handles( num_elements );
    Teuchos::ArrayRCP<int> tet_connectivity( 4*num_elements );
    int elem_idx, vertex_idx;
    int v0, v1, v2, v3, v4, v5, v6, v7;
    for ( int j = 0; j < (edge_length-1); ++j )
    {
	for ( int i = 0; i < (edge_length-1); ++i )
	{
	    // Indices.
	    vertex_idx = i + j*edge_length;
	    v0 = vertex_idx;
	    v1 = vertex_idx + 1;
	    v2 = vertex_idx + 1 + edge_length;
	    v3 = vertex_idx +     edge_length;
	    v4 = vertex_idx +                   num_vertices/2;
	    v5 = vertex_idx + 1 +               num_vertices/2;
	    v6 = vertex_idx + 1 + edge_length + num_vertices/2;
	    v7 = vertex_idx +     edge_length + num_vertices/2; 

	    // Tetrahedron 1.
	    elem_idx = i + j*(edge_length-1);
	    tet_handles[elem_idx] = elem_idx + elem_offset;
	    tet_connectivity[elem_idx]                = vertex_handles[v0];
	    tet_connectivity[num_elements+elem_idx]   = vertex_handles[v1];
	    tet_connectivity[2*num_elements+elem_idx] = vertex_handles[v3];
	    tet_connectivity[3*num_elements+elem_idx] = vertex_handles[v4];

	    // Tetrahedron 2.
	    elem_idx = i + j*(edge_length-1) + num_elements/5;
	    tet_handles[elem_idx] = elem_idx + elem_offset;
	    tet_connectivity[elem_idx] 	              = vertex_handles[v1];
	    tet_connectivity[num_elements+elem_idx]   = vertex_handles[v2];
	    tet_connectivity[2*num_elements+elem_idx] = vertex_handles[v3];
	    tet_connectivity[3*num_elements+elem_idx] = vertex_handles[v6];

	    // Tetrahedron 3.
	    elem_idx = i + j*(edge_length-1) + 2*num_elements/5;
	    tet_handles[elem_idx] = elem_idx + elem_offset;
	    tet_connectivity[elem_idx] 	              = vertex_handles[v6];
	    tet_connectivity[num_elements+elem_idx]   = vertex_handles[v5];
	    tet_connectivity[2*num_elements+elem_idx] = vertex_handles[v4];
	    tet_connectivity[3*num_elements+elem_idx] = vertex_handles[v1];

	    // Tetrahedron 4.
	    elem_idx = i + j*(edge_length-1) + 3*num_elements/5;
	    tet_handles[elem_idx] = elem_idx + elem_offset;
	    tet_connectivity[elem_idx]   	      = vertex_handles[v4];
	    tet_connectivity[num_elements+elem_idx]   = vertex_handles[v7];
	    tet_connectivity[2*num_elements+elem_idx] = vertex_handles[v6];
	    tet_connectivity[3*num_elements+elem_idx] = vertex_handles[v3];

	    // Tetrahedron 5.
	    elem_idx = i + j*(edge_length-1) + 4*num_elements/5;
	    tet_handles[elem_idx] = elem_idx + elem_offset;
	    tet_connectivity[elem_idx] 	              = vertex_handles[v3];
	    tet_connectivity[num_elements+elem_idx]   = vertex_handles[v1];
	    tet_connectivity[2*num_elements+elem_idx] = vertex_handles[v6];
	    tet_connectivity[3*num_elements+elem_idx] = vertex_handles[v4];
	}
    }

    Teuchos::ArrayRCP<int> permutation_list( 4 );
    for ( int i = 0; i < permutation_list.size(); ++i )
    {
	permutation_list[i] = i;
    }

    return Teuchos::rcp( 
	new DataTransferKit::MeshContainer<int>( 3, vertex_handles, coords, 
						 DataTransferKit::DTK_TETRAHEDRON, 4,
						 tet_handles, tet_connectivity,
						 permutation_list ) );
}

//---------------------------------------------------------------------------//
Teuchos::RCP<DataTransferKit::MeshContainer<int> > buildNullTetMesh()
{
    Teuchos::ArrayRCP<int> vertex_handles(0);
    Teuchos::ArrayRCP<double> coords(0);
    Teuchos::ArrayRCP<int> tet_handles(0);
    Teuchos::ArrayRCP<int> tet_connectivity(0);
    Teuchos::ArrayRCP<int> permutation_list(4);
    for ( int i = 0; (int) i < permutation_list.size(); ++i )
    {
	permutation_list[i] = i;
    }

    return Teuchos::rcp( 
	new DataTransferKit::MeshContainer<int>( 3, vertex_handles, coords, 
						 DataTransferKit::DTK_TETRAHEDRON, 4,
						 tet_handles, tet_connectivity,
						 permutation_list ) );
}

//---------------------------------------------------------------------------//
Teuchos::RCP<DataTransferKit::MeshContainer<int> >  
buildHexMesh( int my_rank, int my_size, int edge_length, int elem_offset )
{
    // Make some vertices.
    int num_vertices = edge_length*edge_length*2;
    int vertex_dim = 3;
    Teuchos::ArrayRCP<int> vertex_handles( num_vertices );
    Teuchos::ArrayRCP<double> coords( vertex_dim*num_vertices );
    int idx;
    for ( int j = 0; j < edge_length; ++j )
    {
	for ( int i = 0; i < edge_length; ++i )
	{
	    idx = i + j*edge_length;
	    vertex_handles[ idx ] = (int) num_vertices*my_rank + idx + elem_offset;
	    coords[ idx ] = i;
	    coords[ num_vertices + idx ] = j;
	    coords[ 2*num_vertices + idx ] = my_rank;
	}
    }
    for ( int j = 0; j < edge_length; ++j )
    {
	for ( int i = 0; i < edge_length; ++i )
	{
	    idx = i + j*edge_length + num_vertices / 2;
	    vertex_handles[ idx ] = (int) num_vertices*my_rank + idx + elem_offset;
	    coords[ idx ] = i;
	    coords[ num_vertices + idx ] = j;
	    coords[ 2*num_vertices + idx ] = my_rank + 1;
	}
    }
    
    // Make the hexahedrons. 
    int num_elements = (edge_length-1)*(edge_length-1);
    Teuchos::ArrayRCP<int> hex_handles( num_elements );
    Teuchos::ArrayRCP<int> hex_connectivity( 8*num_elements );
    int elem_idx, vertex_idx;
    for ( int j = 0; j < (edge_length-1); ++j )
    {
	for ( int i = 0; i < (edge_length-1); ++i )
	{
	    vertex_idx = i + j*edge_length;
	    elem_idx = i + j*(edge_length-1);

	    hex_handles[elem_idx] = elem_idx + elem_offset;

	    hex_connectivity[elem_idx] 
		= vertex_handles[vertex_idx];

	    hex_connectivity[num_elements+elem_idx] 
		= vertex_handles[vertex_idx+1];

	    hex_connectivity[2*num_elements+elem_idx] 
		= vertex_handles[vertex_idx+edge_length+1];

	    hex_connectivity[3*num_elements+elem_idx] 
		= vertex_handles[vertex_idx+edge_length];

	    hex_connectivity[4*num_elements+elem_idx] 
		= vertex_handles[vertex_idx+num_vertices/2];

	    hex_connectivity[5*num_elements+elem_idx] 
		= vertex_handles[vertex_idx+num_vertices/2+1];

 	    hex_connectivity[6*num_elements+elem_idx] 
		= vertex_handles[vertex_idx+num_vertices/2+edge_length+1];

	    hex_connectivity[7*num_elements+elem_idx] 
		= vertex_handles[vertex_idx+num_vertices/2+edge_length];
	}
    }

    Teuchos::ArrayRCP<int> permutation_list( 8 );
    for ( int i = 0; i < permutation_list.size(); ++i )
    {
	permutation_list[i] = i;
    }

    return Teuchos::rcp( 
	new DataTransferKit::MeshContainer<int>( 3, vertex_handles, coords, 
						 DataTransferKit::DTK_HEXAHEDRON, 8,
						 hex_handles, hex_connectivity,
						 permutation_list ) );
}

//---------------------------------------------------------------------------//
Teuchos::RCP<DataTransferKit::MeshContainer<int> > 
buildNullHexMesh()
{
    Teuchos::ArrayRCP<int> vertex_handles(0);
    Teuchos::ArrayRCP<double> coords(0);
    Teuchos::ArrayRCP<int> hex_handles(0);
    Teuchos::ArrayRCP<int> hex_connectivity(0);
    Teuchos::ArrayRCP<int> permutation_list(8);
    for ( int i = 0; (int) i < permutation_list.size(); ++i )
    {
	permutation_list[i] = i;
    }

    return Teuchos::rcp( 
	new DataTransferKit::MeshContainer<int>( 3, vertex_handles, coords, 
						 DataTransferKit::DTK_HEXAHEDRON, 8,
						 hex_handles, hex_connectivity,
						 permutation_list ) );
}

//---------------------------------------------------------------------------//
Teuchos::RCP<DataTransferKit::MeshContainer<int> >  
buildPyramidMesh( int my_rank, int my_size, int edge_length, int elem_offset )
{
    // Make some vertices.
    int num_vertices = edge_length*edge_length*2 + (edge_length-1)*(edge_length-1);
    int vertex_dim = 3;
    Teuchos::ArrayRCP<int> vertex_handles( num_vertices );
    Teuchos::ArrayRCP<double> coords( vertex_dim*num_vertices );
    int idx;
    for ( int j = 0; j < edge_length; ++j )
    {
	for ( int i = 0; i < edge_length; ++i )
	{
	    idx = i + j*edge_length;
	    vertex_handles[ idx ] = (int) num_vertices*my_rank + idx + elem_offset;
	    coords[ idx ] = i;
	    coords[ num_vertices + idx ] = j;
	    coords[ 2*num_vertices + idx ] = my_rank;
	}
    }
    for ( int j = 0; j < edge_length; ++j )
    {
	for ( int i = 0; i < edge_length; ++i )
	{
	    idx = i + j*edge_length + edge_length*edge_length;
	    vertex_handles[ idx ] = (int) num_vertices*my_rank + idx + elem_offset;
	    coords[ idx ] = i;
	    coords[ num_vertices + idx ] = j;
	    coords[ 2*num_vertices + idx ] = my_rank+1;
	}
    }
    for ( int j = 0; j < edge_length-1; ++j )
    {
	for ( int i = 0; i < edge_length-1; ++i )
	{
	    idx = i + j*(edge_length-1) + edge_length*edge_length*2;
	    vertex_handles[ idx ] = (int) num_vertices*my_rank + idx + elem_offset;
	    coords[ idx ] = i + 0.5;
	    coords[ num_vertices + idx ] = j + 0.5;
	    coords[ 2*num_vertices + idx ] = my_rank + 0.5;
	}
    }
    
    // Make the pyramids. 
    int num_elements = (edge_length-1)*(edge_length-1)*6;
    Teuchos::ArrayRCP<int> pyr_handles( num_elements );
    Teuchos::ArrayRCP<int> pyr_connectivity( 5*num_elements );
    int elem_idx, vertex_idx;
    int v0, v1, v2, v3, v4, v5, v6, v7, v8;
    for ( int j = 0; j < (edge_length-1); ++j )
    {
	for ( int i = 0; i < (edge_length-1); ++i )
	{
	    // Indices.
	    vertex_idx = i + j*edge_length;
	    v0 = vertex_idx;
	    v1 = vertex_idx + 1;
	    v2 = vertex_idx + 1 + edge_length;
	    v3 = vertex_idx +     edge_length;
	    v4 = vertex_idx +                   edge_length*edge_length;
	    v5 = vertex_idx + 1 +               edge_length*edge_length;
	    v6 = vertex_idx + 1 + edge_length + edge_length*edge_length;
	    v7 = vertex_idx +     edge_length + edge_length*edge_length;
	    v8 = i + j*(edge_length-1) + edge_length*edge_length*2;

	    // Pyramid 1.
	    elem_idx = i + j*(edge_length-1);
	    pyr_handles[elem_idx] = elem_idx + elem_offset;
	    pyr_connectivity[elem_idx]                = vertex_handles[v0];
	    pyr_connectivity[num_elements+elem_idx]   = vertex_handles[v1];
	    pyr_connectivity[2*num_elements+elem_idx] = vertex_handles[v2];
	    pyr_connectivity[3*num_elements+elem_idx] = vertex_handles[v3];
	    pyr_connectivity[4*num_elements+elem_idx] = vertex_handles[v8];

	    // Pyramid 2.
	    elem_idx = i + j*(edge_length-1) + num_elements/6;
	    pyr_handles[elem_idx] = elem_idx + elem_offset;
	    pyr_connectivity[elem_idx] 	              = vertex_handles[v1];
	    pyr_connectivity[num_elements+elem_idx]   = vertex_handles[v5];
	    pyr_connectivity[2*num_elements+elem_idx] = vertex_handles[v6];
	    pyr_connectivity[3*num_elements+elem_idx] = vertex_handles[v2];
	    pyr_connectivity[4*num_elements+elem_idx] = vertex_handles[v8];

	    // Pyramid 3.
	    elem_idx = i + j*(edge_length-1) + 2*num_elements/6;
	    pyr_handles[elem_idx] = elem_idx + elem_offset;
	    pyr_connectivity[elem_idx] 	              = vertex_handles[v2];
	    pyr_connectivity[num_elements+elem_idx]   = vertex_handles[v6];
	    pyr_connectivity[2*num_elements+elem_idx] = vertex_handles[v7];
	    pyr_connectivity[3*num_elements+elem_idx] = vertex_handles[v3];
	    pyr_connectivity[4*num_elements+elem_idx] = vertex_handles[v8];

	    // Pyramid 4.
	    elem_idx = i + j*(edge_length-1) + 3*num_elements/6;
	    pyr_handles[elem_idx] = elem_idx + elem_offset;
	    pyr_connectivity[elem_idx]   	      = vertex_handles[v4];
	    pyr_connectivity[num_elements+elem_idx]   = vertex_handles[v0];
	    pyr_connectivity[2*num_elements+elem_idx] = vertex_handles[v3];
	    pyr_connectivity[3*num_elements+elem_idx] = vertex_handles[v7];
	    pyr_connectivity[4*num_elements+elem_idx] = vertex_handles[v8];

	    // Pyramid 5.
	    elem_idx = i + j*(edge_length-1) + 4*num_elements/6;
	    pyr_handles[elem_idx] = elem_idx + elem_offset;
	    pyr_connectivity[elem_idx]   	      = vertex_handles[v4];
	    pyr_connectivity[num_elements+elem_idx]   = vertex_handles[v5];
	    pyr_connectivity[2*num_elements+elem_idx] = vertex_handles[v1];
	    pyr_connectivity[3*num_elements+elem_idx] = vertex_handles[v0];
	    pyr_connectivity[4*num_elements+elem_idx] = vertex_handles[v8];

	    // Pyramid 6.
	    elem_idx = i + j*(edge_length-1) + 5*num_elements/6;
	    pyr_handles[elem_idx] = elem_idx + elem_offset;
	    pyr_connectivity[elem_idx]   	      = vertex_handles[v4];
	    pyr_connectivity[num_elements+elem_idx]   = vertex_handles[v7];
	    pyr_connectivity[2*num_elements+elem_idx] = vertex_handles[v6];
	    pyr_connectivity[3*num_elements+elem_idx] = vertex_handles[v5];
	    pyr_connectivity[4*num_elements+elem_idx] = vertex_handles[v8];
	}
    }

    Teuchos::ArrayRCP<int> permutation_list( 5 );
    for ( int i = 0; i < permutation_list.size(); ++i )
    {
	permutation_list[i] = i;
    }

    return Teuchos::rcp( 
	new DataTransferKit::MeshContainer<int>( 3, vertex_handles, coords, 
						 DataTransferKit::DTK_PYRAMID, 5,
						 pyr_handles, pyr_connectivity,
						 permutation_list ) );
}

//---------------------------------------------------------------------------//
Teuchos::RCP<DataTransferKit::MeshContainer<int> > buildNullPyramidMesh()
{
    Teuchos::ArrayRCP<int> vertex_handles(0);
    Teuchos::ArrayRCP<double> coords(0);
    Teuchos::ArrayRCP<int> pyramid_handles(0);
    Teuchos::ArrayRCP<int> pyramid_connectivity(0);
    Teuchos::ArrayRCP<int> permutation_list(5);
    for ( int i = 0; (int) i < permutation_list.size(); ++i )
    {
	permutation_list[i] = i;
    }

    return Teuchos::rcp( 
	new DataTransferKit::MeshContainer<int>( 3, vertex_handles, coords, 
						 DataTransferKit::DTK_PYRAMID, 5,
						 pyramid_handles, pyramid_connectivity,
						 permutation_list ) );
}

//---------------------------------------------------------------------------//
Teuchos::RCP<DataTransferKit::MeshContainer<int> >  
buildWedgeMesh( int my_rank, int my_size, int edge_length, int elem_offset )
{
    // Make some vertices.
    int num_vertices = edge_length*edge_length*2;
    int vertex_dim = 3;
    Teuchos::ArrayRCP<int> vertex_handles( num_vertices );
    Teuchos::ArrayRCP<double> coords( vertex_dim*num_vertices );
    int idx;
    for ( int j = 0; j < edge_length; ++j )
    {
	for ( int i = 0; i < edge_length; ++i )
	{
	    idx = i + j*edge_length;
	    vertex_handles[ idx ] = (int) num_vertices*my_rank + idx + elem_offset;
	    coords[ idx ] = i;
	    coords[ num_vertices + idx ] = j;
	    coords[ 2*num_vertices + idx ] = my_rank;
	}
    }
    for ( int j = 0; j < edge_length; ++j )
    {
	for ( int i = 0; i < edge_length; ++i )
	{
	    idx = i + j*edge_length + edge_length*edge_length;
	    vertex_handles[ idx ] = (int) num_vertices*my_rank + idx + elem_offset;
	    coords[ idx ] = i;
	    coords[ num_vertices + idx ] = j;
	    coords[ 2*num_vertices + idx ] = my_rank+1;
	}
    }
    
    // Make the wedges. 
    int num_elements = (edge_length-1)*(edge_length-1)*2;
    Teuchos::ArrayRCP<int> wedge_handles( num_elements );
    Teuchos::ArrayRCP<int> wedge_connectivity( 6*num_elements );
    int elem_idx, vertex_idx;
    int v0, v1, v2, v3, v4, v5, v6, v7;
    for ( int j = 0; j < (edge_length-1); ++j )
    {
	for ( int i = 0; i < (edge_length-1); ++i )
	{
	    // Indices.
	    vertex_idx = i + j*edge_length;
	    v0 = vertex_idx;
	    v1 = vertex_idx + 1;
	    v2 = vertex_idx + 1 + edge_length;
	    v3 = vertex_idx +     edge_length;
	    v4 = vertex_idx +                   edge_length*edge_length;
	    v5 = vertex_idx + 1 +               edge_length*edge_length;
	    v6 = vertex_idx + 1 + edge_length + edge_length*edge_length;
	    v7 = vertex_idx +     edge_length + edge_length*edge_length;

	    // Wedge 1.
	    elem_idx = i + j*(edge_length-1);
	    wedge_handles[elem_idx] = elem_idx + elem_offset;
	    wedge_connectivity[elem_idx]                = vertex_handles[v0];
	    wedge_connectivity[num_elements+elem_idx]   = vertex_handles[v4];
	    wedge_connectivity[2*num_elements+elem_idx] = vertex_handles[v1];
	    wedge_connectivity[3*num_elements+elem_idx] = vertex_handles[v3];
	    wedge_connectivity[4*num_elements+elem_idx] = vertex_handles[v7];
	    wedge_connectivity[5*num_elements+elem_idx] = vertex_handles[v2];

	    // Wedge 2.
	    elem_idx = i + j*(edge_length-1) + num_elements/2;
	    wedge_handles[elem_idx] = elem_idx + elem_offset;
	    wedge_connectivity[elem_idx] 	        = vertex_handles[v1];
	    wedge_connectivity[num_elements+elem_idx]   = vertex_handles[v4];
	    wedge_connectivity[2*num_elements+elem_idx] = vertex_handles[v5];
	    wedge_connectivity[3*num_elements+elem_idx] = vertex_handles[v2];
	    wedge_connectivity[4*num_elements+elem_idx] = vertex_handles[v7];
	    wedge_connectivity[5*num_elements+elem_idx] = vertex_handles[v6];
	}
    }

    Teuchos::ArrayRCP<int> permutation_list( 6 );
    for ( int i = 0; i < permutation_list.size(); ++i )
    {
	permutation_list[i] = i;
    }

    return Teuchos::rcp( 
	new DataTransferKit::MeshContainer<int>( 3, vertex_handles, coords, 
						 DataTransferKit::DTK_WEDGE, 6,
						 wedge_handles, wedge_connectivity,
						 permutation_list ) );
}

//---------------------------------------------------------------------------//
Teuchos::RCP<DataTransferKit::MeshContainer<int> > buildNullWedgeMesh()
{
    Teuchos::ArrayRCP<int> vertex_handles(0);
    Teuchos::ArrayRCP<double> coords(0);
    Teuchos::ArrayRCP<int> wedge_handles(0);
    Teuchos::ArrayRCP<int> wedge_connectivity(0);
    Teuchos::ArrayRCP<int> permutation_list(6);
    for ( int i = 0; (int) i < permutation_list.size(); ++i )
    {
	permutation_list[i] = i;
    }

    return Teuchos::rcp( 
	new DataTransferKit::MeshContainer<int>( 3, vertex_handles, coords, 
						 DataTransferKit::DTK_WEDGE, 6,
						 wedge_handles, wedge_connectivity,
						 permutation_list ) );
}

//---------------------------------------------------------------------------//
// Geometry create functions. These geometries will span the entire domain,
// requiring them to be broadcast throughout the rendezvous.
//---------------------------------------------------------------------------//
void buildCylinderGeometry( 
    int my_size, int edge_size,
    Teuchos::ArrayRCP<DataTransferKit::Cylinder>& cylinders,
    Teuchos::ArrayRCP<int>& gids )
{
    Teuchos::ArrayRCP<DataTransferKit::Cylinder> new_cylinders(1);
    Teuchos::ArrayRCP<int> new_gids(1,0);
    double length = (double) my_size;
    double radius = (double) (edge_size-1) / 2.0;
    double x_center = (double) (edge_size-1) / 2.0;
    double y_center = (double) (edge_size-1) / 2.0;
    double z_center = (double) my_size / 2.0;
    new_cylinders[0] = DataTransferKit::Cylinder( length, radius,
						  x_center, y_center, z_center );
    cylinders = new_cylinders;
    gids = new_gids;
}

//---------------------------------------------------------------------------//
void buildBoxGeometry( int my_size, int edge_size,
		       Teuchos::ArrayRCP<DataTransferKit::Box>& boxes,
		       Teuchos::ArrayRCP<int>& gids )
{
    Teuchos::ArrayRCP<DataTransferKit::Box> new_boxes(1);
    Teuchos::ArrayRCP<int> new_gids(1,0);
    new_boxes[0] = DataTransferKit::Box( 0.0, 0.0, 0.0, edge_size-1,
					 edge_size-1, my_size );
    boxes = new_boxes;
    gids = new_gids;
}

//---------------------------------------------------------------------------//
// Unit tests.
//---------------------------------------------------------------------------//
TEUCHOS_UNIT_TEST( IntegralAssemblyMap, cylinder_test )
{
    using namespace DataTransferKit;
    typedef MeshContainer<int> MeshType;
    typedef MeshTraits<MeshType> MT;

    // Setup communication.
    Teuchos::RCP< const Teuchos::Comm<int> > comm = getDefaultComm<int>();
    int my_rank = comm->getRank();
    int my_size = comm->getSize();

    // Compute element ordinal offsets so we make unique global ordinals.
    int edge_size = 10;
    int tet_offset = 0;
    int hex_offset = tet_offset + (edge_size+1)*(edge_size+1)*5;
    int pyramid_offset = hex_offset + (edge_size+1)*(edge_size+1);
    int wedge_offset = pyramid_offset + (edge_size+1)*(edge_size+1)*6;

    // Setup source mesh manager.
    Teuchos::ArrayRCP<Teuchos::RCP<MeshType> > mesh_blocks( 4 );
    if ( my_rank == 0 )
    {
	mesh_blocks[0] = 
	    buildTetMesh( my_rank, my_size, edge_size, tet_offset );
	mesh_blocks[1] = buildNullHexMesh();
	mesh_blocks[2] = buildNullPyramidMesh();
	mesh_blocks[3] = buildNullWedgeMesh();
    }
    else if ( my_rank == 1 )
    {
	mesh_blocks[0] = buildNullTetMesh();
	mesh_blocks[1] = 
	    buildHexMesh( my_rank, my_size, edge_size, hex_offset );
	mesh_blocks[2] = buildNullPyramidMesh();
	mesh_blocks[3] = buildNullWedgeMesh();
    }
    else if ( my_rank == 2 )
    {
	mesh_blocks[0] = buildNullTetMesh();
	mesh_blocks[1] = buildNullHexMesh();
	mesh_blocks[2] = 
	    buildPyramidMesh( my_rank, my_size, edge_size, pyramid_offset );
	mesh_blocks[3] = buildNullWedgeMesh();
    }
    else if ( my_rank == 3 )
    {
	mesh_blocks[0] = buildNullTetMesh();
	mesh_blocks[1] = buildNullHexMesh();
	mesh_blocks[2] = buildNullPyramidMesh();
	mesh_blocks[3] = 
	    buildWedgeMesh( my_rank, my_size, edge_size, wedge_offset );
    }
    comm->barrier();

    // Create a mesh manager.
    Teuchos::RCP< MeshManager<MeshType> > source_mesh_manager = Teuchos::rcp(
	new MeshManager<MeshType>( mesh_blocks, getDefaultComm<int>(), 3 ) );

    // Setup target.
    int num_geom = 1;
    int geometry_dim = 3;
    Teuchos::ArrayRCP<Cylinder> geometry(0);
    Teuchos::ArrayRCP<int> geom_gids(0);
    int target_dim = 3;
    Teuchos::RCP<MyField> target_field;
    if ( my_rank == 0 )
    {
	buildCylinderGeometry( my_size, edge_size, geometry, geom_gids );
	target_field = 	Teuchos::rcp( new MyField( num_geom, target_dim ) );
    }
    else
    {
	target_field = 	Teuchos::rcp( new MyField( 0, target_dim ) );
    }
    comm->barrier();
    Teuchos::RCP< GeometryManager<Cylinder,int> > target_geometry_manager = 
	Teuchos::rcp( new GeometryManager<Cylinder,int>( 
			  geometry, geom_gids, comm, geometry_dim ) );
    Teuchos::RCP<FieldManager<MyField> > target_space_manager = Teuchos::rcp( 
	new FieldManager<MyField>( target_field, comm ) );

    // Setup source.
    Teuchos::RCP<FieldIntegrator<MeshType ,MyField> > source_integrator;
    Teuchos::RCP<ElementMeasure<MeshType> > source_mesh_measure;
    if ( my_rank == 0 )
    {
    	source_integrator = Teuchos::rcp( new MyIntegrator( *mesh_blocks[0], comm ) );
    	source_mesh_measure = Teuchos::rcp( new MyMeasure( *mesh_blocks[0], comm ) );
    }
    else if ( my_rank == 1 )
    {
    	source_integrator = Teuchos::rcp( new MyIntegrator( *mesh_blocks[1], comm ) );
    	source_mesh_measure = Teuchos::rcp( new MyMeasure( *mesh_blocks[1], comm ) );
    }
    else if ( my_rank == 2 )
    {
    	source_integrator = Teuchos::rcp( new MyIntegrator( *mesh_blocks[2], comm ) );
    	source_mesh_measure = Teuchos::rcp( new MyMeasure( *mesh_blocks[2], comm ) );
    }
    else
    {
    	source_integrator = Teuchos::rcp( new MyIntegrator( *mesh_blocks[3], comm ) );
    	source_mesh_measure = Teuchos::rcp( new MyMeasure( *mesh_blocks[3], comm ) );
    }
    comm->barrier();

    // Setup and apply the integral assembly mapping.
    IntegralAssemblyMap<MeshType,Cylinder> integral_assembly_map( 
	comm, source_mesh_manager->dim(), 1.0e-6, false );
    integral_assembly_map.setup( source_mesh_manager, source_mesh_measure,
				 target_geometry_manager );
    integral_assembly_map.apply( source_integrator, target_space_manager );

    // Check the integration. If a mesh element has a vertex in the cylinder,
    // it should have contributed to the global sum. The global number of
    // elements that intersect the cylinder will equal the integral. Keep in
    // mind this is not a formal conformal mesh, but given that there is only
    // one cylinder across the global domain, we can define how it will
    // behave.
    Cylinder global_cylinder;
    if ( my_rank == 0 )
    {
	global_cylinder = geometry[0];
    }
    comm->barrier();
    Teuchos::broadcast( *comm, 0, Teuchos::Ptr<Cylinder>(&global_cylinder) );

    int num_vertices = MeshTools<MeshType>::numVertices( *mesh_blocks[my_rank] );
    Teuchos::ArrayRCP<const double> coords = 
	MeshTools<MeshType>::coordsView( *mesh_blocks[my_rank] );
    int vertices_per_element = MT::verticesPerElement( *mesh_blocks[my_rank] );
    int num_elements = MeshTools<MeshType>::numElements( *mesh_blocks[my_rank] );
    Teuchos::ArrayRCP<const int> connectivity = 
	MeshTools<MeshType>::connectivityView( *mesh_blocks[my_rank] );
    Teuchos::ArrayRCP<const int> elements = 
	MeshTools<MeshType>::elementsView( *mesh_blocks[my_rank] );

    std::map<int,int> element_g2l;
    for ( int i = 0; i < num_elements; ++i )
    {
	element_g2l[ elements[i] ] = i;
    }

    int vert_index;
    Teuchos::Array<double> vertex(3);
    bool found = false;
    double tol = 1.0e-6;
    int num_in_cylinder = 0;
    for ( int i = 0; i < num_elements; ++i )
    {
	found = false;
	for ( int n = 0; n < vertices_per_element; ++n )
	{
	    if (!found)
	    {
		vert_index = 
		    element_g2l.find(connectivity[i + n*num_elements])->second;
		vertex[0] = coords[vert_index];
		vertex[1] = coords[vert_index + num_vertices];
		vertex[2] = coords[vert_index + 2*num_vertices];

		if ( global_cylinder.pointInCylinder( vertex, tol ) )
		{
		    ++num_in_cylinder;
		    found = true;
		}
	    }
	}
    }
    comm->barrier();

    int global_num_in_cylinder = 0;
    Teuchos::reduceAll( *comm, Teuchos::REDUCE_SUM,
			num_in_cylinder, Teuchos::Ptr<int>(&global_num_in_cylinder) );

    if ( my_rank == 0 )
    {
	for ( int d = 0; d < target_dim; ++d )
	{
	    TEST_ASSERT( 2.0 == target_field->getData()[d] );
	}
    }
    comm->barrier();
}

//---------------------------------------------------------------------------//
TEUCHOS_UNIT_TEST( IntegralAssemblyMap, box_test )
{
    using namespace DataTransferKit;
    typedef MeshContainer<int> MeshType;
    typedef MeshTraits<MeshType> MT;

    // Setup communication.
    Teuchos::RCP< const Teuchos::Comm<int> > comm = getDefaultComm<int>();
    int my_rank = comm->getRank();
    int my_size = comm->getSize();

    // Compute element ordinal offsets so we make unique global ordinals.
    int edge_size = 10;
    int tet_offset = 0;
    int hex_offset = tet_offset + (edge_size+1)*(edge_size+1)*5;
    int pyramid_offset = hex_offset + (edge_size+1)*(edge_size+1);
    int wedge_offset = pyramid_offset + (edge_size+1)*(edge_size+1)*6;

    // Setup source mesh manager.
    Teuchos::ArrayRCP<Teuchos::RCP<MeshType> > mesh_blocks( 4 );
    if ( my_rank == 0 )
    {
	mesh_blocks[0] = 
	    buildTetMesh( my_rank, my_size, edge_size, tet_offset );
	mesh_blocks[1] = buildNullHexMesh();
	mesh_blocks[2] = buildNullPyramidMesh();
	mesh_blocks[3] = buildNullWedgeMesh();
    }
    else if ( my_rank == 1 )
    {
	mesh_blocks[0] = buildNullTetMesh();
	mesh_blocks[1] = 
	    buildHexMesh( my_rank, my_size, edge_size, hex_offset );
	mesh_blocks[2] = buildNullPyramidMesh();
	mesh_blocks[3] = buildNullWedgeMesh();
    }
    else if ( my_rank == 2 )
    {
	mesh_blocks[0] = buildNullTetMesh();
	mesh_blocks[1] = buildNullHexMesh();
	mesh_blocks[2] = 
	    buildPyramidMesh( my_rank, my_size, edge_size, pyramid_offset );
	mesh_blocks[3] = buildNullWedgeMesh();
    }
    else if ( my_rank == 3 )
    {
	mesh_blocks[0] = buildNullTetMesh();
	mesh_blocks[1] = buildNullHexMesh();
	mesh_blocks[2] = buildNullPyramidMesh();
	mesh_blocks[3] = 
	    buildWedgeMesh( my_rank, my_size, edge_size, wedge_offset );
    }
    comm->barrier();

    // Create a mesh manager.
    Teuchos::RCP< MeshManager<MeshType> > source_mesh_manager = Teuchos::rcp(
	new MeshManager<MeshType>( mesh_blocks, getDefaultComm<int>(), 3 ) );

    // Setup target.
    int num_geom = 1;
    int geometry_dim = 3;
    Teuchos::ArrayRCP<Box> geometry(0);
    Teuchos::ArrayRCP<int> geom_gids(0);
    int target_dim = 3;
    Teuchos::RCP<MyField> target_field;
    if ( my_rank == 0 )
    {
	buildBoxGeometry( my_size, edge_size, geometry, geom_gids );
	target_field = 	Teuchos::rcp( new MyField( num_geom, target_dim ) );
    }
    else
    {
	target_field = 	Teuchos::rcp( new MyField( 0, target_dim ) );
    }
    comm->barrier();
    Teuchos::RCP< GeometryManager<Box,int> > target_geometry_manager =
	Teuchos::rcp( new GeometryManager<Box,int>( 
			  geometry, geom_gids, comm, geometry_dim ) );
    Teuchos::RCP<FieldManager<MyField> > target_space_manager = Teuchos::rcp( 
	new FieldManager<MyField>( target_field, comm ) );

    // Create field integrator and element measure.
    Teuchos::RCP< FieldIntegrator<MeshType ,MyField> > source_integrator;
    Teuchos::RCP<ElementMeasure<MeshType> > source_mesh_measure;
    if ( my_rank == 0 )
    {
    	source_integrator = Teuchos::rcp( new MyIntegrator( *mesh_blocks[0], comm ) );
    	source_mesh_measure = Teuchos::rcp( new MyMeasure( *mesh_blocks[0], comm ) );
    }
    else if ( my_rank == 1 )
    {
    	source_integrator = Teuchos::rcp( new MyIntegrator( *mesh_blocks[1], comm ) );
    	source_mesh_measure = Teuchos::rcp( new MyMeasure( *mesh_blocks[1], comm ) );
    }
    else if ( my_rank == 2 )
    {
    	source_integrator = Teuchos::rcp( new MyIntegrator( *mesh_blocks[2], comm ) );
    	source_mesh_measure = Teuchos::rcp( new MyMeasure( *mesh_blocks[2], comm ) );
    }
    else
    {
    	source_integrator = Teuchos::rcp( new MyIntegrator( *mesh_blocks[3], comm ) );
    	source_mesh_measure = Teuchos::rcp( new MyMeasure( *mesh_blocks[3], comm ) );
    }
    comm->barrier();

    // Create data target. This target is a scalar.
    // Setup and apply the integral assembly mapping.
    IntegralAssemblyMap<MeshType,Box> integral_assembly_map( 
	comm, source_mesh_manager->dim(), 1.0e-6, false );
    integral_assembly_map.setup( source_mesh_manager, source_mesh_measure,
				 target_geometry_manager );
    integral_assembly_map.apply( source_integrator, target_space_manager );

    // Check the integration. All elements in the mesh are in the box as this
    // is a true conformal situation and therefore the total integral should
    // be the global number of mesh elements. 
    if ( my_rank == 0 )
    {
	for ( int d = 0; d < target_dim; ++d )
	{
	    TEST_ASSERT( 2.0 == target_field->getData()[d] );
	}
    }
    comm->barrier();
}

//---------------------------------------------------------------------------//
// end tstIntegralAssemblyMap1.cpp
//---------------------------------------------------------------------------//

