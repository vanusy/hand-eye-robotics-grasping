// CSV2PCD.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <pcl/visualization/cloud_viewer.h>
#include <iostream>
#include <pcl/io/io.h>
#include <pcl/io/pcd_io.h>
#include <math.h>

#define DISTANCE_TOLERANCE 1.0

int user_data;


/**
 * Calculate the determinant for a 4x4 matrix based on this example:
 * http://www.euclideanspace.com/maths/algebra/matrix/functions/determinant/fourD/index.htm
 * This function takes four Vec4f as row vectors and calculates the resulting matrix' determinant
 * using the Laplace expansion.
 *
 */
inline const float Determinant4x4( const pcl::PointXYZ& v0,
                                   const pcl::PointXYZ& v1,
                                   const pcl::PointXYZ& v2,
                                   const pcl::PointXYZ& v3 )
{
float det = 1*v1.z*v2.y*v3.x - v0.z*1*v2.y*v3.x -
            1*v1.y*v2.z*v3.x + v0.y*1*v2.z*v3.x +
 
            v0.z*v1.y*1*v3.x - v0.y*v1.z*1*v3.x -
            1*v1.z*v2.x*v3.y + v0.z*1*v2.x*v3.y +
 
            1*v1.x*v2.z*v3.y - v0.x*1*v2.z*v3.y -
            v0.z*v1.x*1*v3.y + v0.x*v1.z*1*v3.y +
 
            1*v1.y*v2.x*v3.z - v0.y*1*v2.x*v3.z -
            1*v1.x*v2.y*v3.z + v0.x*1*v2.y*v3.z +
 
            v0.y*v1.x*1*v3.z - v0.x*v1.y*1*v3.z -
            v0.z*v1.y*v2.x*1 + v0.y*v1.z*v2.x*1 +
 
            v0.z*v1.x*v2.y*1 - v0.x*v1.z*v2.y*1 -
            v0.y*v1.x*v2.z*1 + v0.x*v1.y*v2.z*1;
    
	return det;
}
 

/* Checks whether the specified point is inside the tetrahedron (by index)
*  based on this approach: http://steve.hollasch.net/cgindex/geometry/ptintet.html
*  @return true if the point_ is inside the tetrahedron (or on one of the four triangles), otherwise false
*  @param point_ the Vec3f point to be checked
*  @param tetra_ the tetrahedron
*/
inline const bool CheckPointInTetra(const pcl::PointXYZ& v0,
									const pcl::PointXYZ& v1,
									const pcl::PointXYZ& v2,
									const pcl::PointXYZ& v3,
									const pcl::PointXYZ& p0 )
{
    const float det0 = Determinant4x4(v0, v1, v2, v3);
    const float det1 = Determinant4x4(p0, v1, v2, v3);
    const float det2 = Determinant4x4(v0, p0, v2, v3);
    const float det3 = Determinant4x4(v0, v1, p0, v3);
    const float det4 = Determinant4x4(v0, v1, v2, p0);

	//printf("det0: %f, det1: %f, det2: %f, det3: %f, det4: %f\n", det0, det1, det2, det3, det4);

    /**
    If by chance the Determinant det0 is 0, then your tetrahedron is degenerate (the points are coplanar).
    If any other Di=0, then P lies on boundary i (boundary i being that boundary formed by the three points other than Vi).
    If the sign of any Di differs from that of D0 then P is outside boundary i.
    If the sign of any Di equals that of D0 then P is inside boundary i.
    If P is inside all 4 boundaries, then it is inside the tetrahedron.
    As a check, it must be that D0 = D1+D2+D3+D4.
    The pattern here should be clear; the computations can be extended to simplicies of any dimension. (The 2D and 3D case are the triangle and the tetrahedron).
    If it is meaningful to you, the quantities bi = Di/D0 are the usual barycentric coordinates.
    Comparing signs of Di and D0 is only a check that P and Vi are on the same side of boundary i.
    */
    if (det0 != 0)
    {
        if (det0 < 0)
        {
            if ((det1 < 0) && (det2 < 0) && (det3 < 0) && (det4 < 0))
            {
                return true;
            }
        }
        if (det0 > 0)
        {
            if ((det1 > 0) && (det2 > 0) && (det3 > 0) && (det4 > 0))
            {
                return true;
            }
        }
    }

    return false;
}

inline float getDistance(const pcl::PointXYZ& p1, const pcl::PointXYZ& p2)  //tested
{
	float fdffx = p1.x-p2.x;
	float fdffy = p1.y-p2.y;
	float fdffz = p1.z-p2.z;
	float fdistancesquare = fdffx*fdffx + fdffy*fdffy + fdffz*fdffz; 
	return std::sqrt(fdistancesquare);
}

inline float getNorm(const pcl::PointXYZ& p) //tested
{
	float fnormsquare = p.x*p.x + p.y*p.y + p.z*p.z; 
	return std::sqrt(fnormsquare);
}

//p1 is projected onto p2
inline float getProjection(const pcl::PointXYZ& p1, const pcl::PointXYZ& p2) //tested
{
	float fdotproduct = p1.x * p2.x + p1.y * p2.y + p1.z * p2.z;
	float fnorm_p2 = getNorm(p2);
	return fdotproduct/fnorm_p2;
}

void viewerOneOff (pcl::visualization::PCLVisualizer& viewer)
{
	viewer.setBackgroundColor (1.0, 0.5, 1.0);
	pcl::PointXYZ o;
	o.x = 1.0;
	o.y = 0;
	o.z = 0;
	viewer.addSphere (o, 0.25, "sphere", 0);
	std::cout << "i only run once" << std::endl;

}

void viewerPsycho (pcl::visualization::PCLVisualizer& viewer)
{
	static unsigned count = 0;
	std::stringstream ss;
	ss << "Once per viewer loop: " << count++;
	viewer.removeShape ("text", 0);
	viewer.addText (ss.str(), 200, 300, "text", 0);

	//FIXME: possible race condition here:
	user_data++;
}

int main ()
{
	/*
	pcl::PointXYZ vertex0;
	pcl::PointXYZ vertex1;
	pcl::PointXYZ vertex2;
	pcl::PointXYZ vertex3;
	pcl::PointXYZ testp1;
	pcl::PointXYZ testp2;
	pcl::PointXYZ testp3;
	pcl::PointXYZ testp4;

	vertex0.x = 0.;
	vertex0.y = 0.;
	vertex0.z = 0.;

	vertex1.x = 1.;
	vertex1.y = 0.;
	vertex1.z = 0.;

	vertex2.x = 0.;
	vertex2.y = 1.;
	vertex2.z = 0.;

	vertex3.x = 0.;
	vertex3.y = 0.;
	vertex3.z = 1.;

	testp1.x = 1./3.; //inside
	testp1.y = 1./3.;
	testp1.z = 1./3.1;

	testp2.x = -1.;   //outside
	testp2.y = -1.;
	testp2.z = 3.;

	testp3.x = 0.;    //outside
	testp3.y = 0.;
	testp3.z = 0.;

	testp4.x = 1./2.; //outside
	testp4.y = 1./2.;
	testp4.z = 0.;

	printf(CheckPointInTetra(vertex0, vertex1, vertex2, vertex3, testp1)? "P1 inside?: true\n" : "P1 inside?: false\n");
	printf(CheckPointInTetra(vertex0, vertex1, vertex2, vertex3, testp2)? "P2 inside?: true\n" : "P2 inside?: false\n");
	printf(CheckPointInTetra(vertex0, vertex1, vertex2, vertex3, testp3)? "P3 inside?: true\n" : "P3 inside?: false\n");
	printf(CheckPointInTetra(vertex0, vertex1, vertex2, vertex3, testp4)? "P4 inside?: true\n" : "P4 inside?: false\n");

	while(1);
	*/

	//printf("sqrt(100.) = %f\n", std::sqrt(100.));

	//while(1);

	//pcl::PointCloud<pcl::PointXYZRGBA>::Ptr cloud (new pcl::PointCloud<pcl::PointXYZRGBA>);
	pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_xyz (new pcl::PointCloud<pcl::PointXYZ>);
	pcl::io::loadPCDFile ("pcd_world_filtered_3.pcd", *cloud_xyz);
	
/*
	pcl::visualization::CloudViewer viewer("Cloud Viewer");

	//blocks until the cloud is actually rendered
	viewer.showCloud(cloud);

	//use the following functions to get access to the underlying more advanced/powerful
	//PCLVisualizer

	//This will only get called once
	viewer.runOnVisualizationThreadOnce (viewerOneOff);

	//This will get called once per visualization iteration
	viewer.runOnVisualizationThread (viewerPsycho);
	
	while (!viewer.wasStopped ())
	{
		//you can also do cool processing here
		//FIXME: Note that this is running in a separate thread from viewerPsycho
		//and you should guardagainst race conditions yourself...
		user_data++;
	}
*/
	pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud_xyzrgb (new pcl::PointCloud<pcl::PointXYZRGB>);
	//pcl::PointCloud<pcl::PointXYZRGB> cloud_xyzrgb;
	cloud_xyzrgb->width    = cloud_xyz->points.size();
	cloud_xyzrgb->height   = 1;
	cloud_xyzrgb->is_dense = true;
	cloud_xyzrgb->points.resize (cloud_xyzrgb->width * cloud_xyzrgb->height);
	
	/////////////////////////////////////////////////////////
	float fAccScore = 0;  //accumulated score

	//Final configuration
	pcl::PointXYZ EndEffector;
	pcl::PointXYZ Finger1;
	pcl::PointXYZ Finger2;
	pcl::PointXYZ Finger3;

	EndEffector.x = 0.582272*1000.;
	EndEffector.y = 0.0115389*1000.;
	EndEffector.z = 1.10896*1000.;

	Finger1.x = 0.834844*1000.;
	Finger1.y = 0.195511*1000.;
	Finger1.z = 0.782682*1000.;

	Finger2.x = 0.846945*1000.;
	Finger2.y = 0.148117*1000.;
	Finger2.z = 0.765035*1000.;

	Finger3.x = 0.698168*1000.;
	Finger3.y = 0.163494*1000.;
	Finger3.z = 0.698335*1000.;

	pcl::PointXYZ FingerCenter;
	FingerCenter.x = (Finger1.x + Finger2.x + Finger3.x)/3.;
	FingerCenter.y = (Finger1.y + Finger2.y + Finger3.y)/3.;
	FingerCenter.z = (Finger1.z + Finger2.z + Finger3.z)/3.;

	///Base vector
	pcl::PointXYZ BaseVector;
	BaseVector.x = FingerCenter.x - EndEffector.x;
	BaseVector.y = FingerCenter.y - EndEffector.y;
	BaseVector.z = FingerCenter.z - EndEffector.z;

	float fNorm_BaseVector = getNorm(BaseVector);
	float fRadius = DISTANCE_TOLERANCE*getDistance(FingerCenter, EndEffector);
	
	printf("Base vector: %f,  %f,  %f\n", BaseVector.x, BaseVector.y, BaseVector.z);
	printf("Norm of base vector: %f mm\n", fNorm_BaseVector);
	printf("Radius: %f mm\n", fRadius);

	/*
	pcl::PointXYZ TestProjectionVector;
	TestProjectionVector.x = 0.241904*1000.;
	TestProjectionVector.y = -0.518697*1000.;
	TestProjectionVector.z = -0.379353*1000.;
	float fPorj_test = getProjection(TestProjectionVector, BaseVector); //TestVector projects onto BaseVector
	printf("Projection result: %f \n", fPorj_test);
	*/

	//while(1);

	///Test vector
	pcl::PointXYZ TestVector;
	long inum_valid_point = 0;

	for (size_t i = 0; i < cloud_xyz->points.size(); ++i)
	{
		cloud_xyzrgb->points[i].x = cloud_xyz->points[i].x;
		cloud_xyzrgb->points[i].y = cloud_xyz->points[i].y;
		cloud_xyzrgb->points[i].z = cloud_xyz->points[i].z;
		cloud_xyzrgb->points[i].r = 255;
		cloud_xyzrgb->points[i].g = 255;
		cloud_xyzrgb->points[i].b = 255;

		if( CheckPointInTetra(Finger1, Finger2, Finger3, EndEffector, cloud_xyz->points[i]) == true )
		{
			TestVector.x = cloud_xyz->points[i].x - FingerCenter.x;
			TestVector.y = cloud_xyz->points[i].y - FingerCenter.y;
			TestVector.z = cloud_xyz->points[i].z - FingerCenter.z;

			float fPorjection = getProjection(TestVector, BaseVector); //TestVector projects onto BaseVector
			fAccScore += (-fPorjection);

			cloud_xyzrgb->points[i].r = 0;
			cloud_xyzrgb->points[i].g = 0;

			inum_valid_point++;
		}
		else
		{ }
	}

	printf("Score: %f\n", fAccScore);
	printf("Number of valid points: %d\n", inum_valid_point);

	pcl::visualization::CloudViewer viewer("Cloud Viewer");

	//blocks until the cloud is actually rendered
	viewer.showCloud(cloud_xyzrgb);

	//use the following functions to get access to the underlying more advanced/powerful
	//PCLVisualizer

	//This will only get called once
	viewer.runOnVisualizationThreadOnce (viewerOneOff);

	//This will get called once per visualization iteration
	viewer.runOnVisualizationThread (viewerPsycho);

	while (!viewer.wasStopped ())
	{
		//you can also do cool processing here
		//FIXME: Note that this is running in a separate thread from viewerPsycho
		//and you should guardagainst race conditions yourself...
		user_data++;
	}
	return 0;
}