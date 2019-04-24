/*
 * io.cpp
 *
 *  Created on: Oct 6, 2018
 *      Author: fabricio
 */

#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <stdio.h>

#include "io.h"
#include "model.h"
#include "paraview.h"

namespace IO {

	static std::string input_fname;
	static std::string out_fname;
	static std::string path_out_fname;
	static std::string path;
	static std::string line;

	std::ifstream inputfile;
	std::ofstream outfile;

	static bool smesh_material_falg = false;

	//** statics

	static void read_dem_horizonts ()
	{
		std::string auxline;
		
		// horizon ID (material ID)
		int hor_id = 0;
		std::getline(inputfile,auxline);
		std::sscanf(auxline.c_str(),"%d",&hor_id);
		
		// get number of lines
		int nHpnts = 0;
		std::getline(inputfile,auxline);
		std::sscanf(auxline.c_str(),"%d",&nHpnts);

		std::vector<Model::HorizontPoint>& horvector = Model::GetHorizontVector();

		if(nHpnts==0){
			std::cout<<"ERROR: number of Horizonts is = "<<nHpnts<<"\n";
			return;
		}

		for( int i = 0; i < nHpnts; i++ )
		{
			std::getline(inputfile,auxline);
			Model::HorizontPoint ihpnt;
			std::sscanf(auxline.c_str(),"%lf%lf%lf%d", &ihpnt.pos.x,&ihpnt.pos.y,&ihpnt.pos.z,&ihpnt.matid);
			horvector.push_back(ihpnt);
		}
	}

	static void read_dem_total_horizonts()
	{
		std::string auxline;
		std::getline(inputfile,auxline);

		// read n horizonts
		int nhor = 0;
		std::sscanf(auxline.c_str(),"%d",&nhor);

		// put the horizon number in struct
		Model::SetHorizonNumber(nhor);
	}

	static void read_dem2mpm()
	{
		std::string auxline;
		std::getline(inputfile,auxline);
		
		// get lines number
		int nbox = 0;
		std::sscanf(auxline.c_str(),"%d",&nbox);
		
		// read lines
		std::getline(inputfile,auxline);

		std::vector<Model::DemBox>& demvector = Model::GetDemVector();

		if(nbox==0){
			std::cout<<"ERROR: number of DEM box is = "<<nbox<<"\n";
			return;
		}

		for( int i = 0; i < nbox; i++ )
		{
			Model::DemBox ibox;
			std::sscanf(auxline.c_str(),"%lf%lf%lf%lf%lf%lf%lf%d", &ibox.pos.x,&ibox.pos.y,&ibox.pos.z,&ibox.zbase,&ibox.dx,&ibox.dy,&ibox.dz,&ibox.matid);
			demvector.push_back(ibox);
			std::getline(inputfile,auxline);
		}
	}

	static void read_smesh_points()
	{
		std::getline(inputfile,line);
		
		// get lines number
		int npnts;	// # of points
		int dim;	// dimension
		int attr;	// attributes
		int bndry;	// # of boundary markers

		std::sscanf(line.c_str(),"%d %d %d %d",&npnts,&dim,&attr,&bndry);

		std::vector<Model::mshPoint>& pntsvector = Model::GetmshPointsVector();

		if(npnts==0){
			std::cout<<"ERROR: number of points is = "<<npnts<<"\n";
			return;
		}

		for( int i = 0; i < npnts; i++ )
		{
			std::getline(inputfile,line);
			Model::mshPoint ipnt;
			std::sscanf(line.c_str(),"%d %lf %lf %lf %d", &ipnt.id,&ipnt.pos.x,&ipnt.pos.y,&ipnt.pos.z,&ipnt.bndry);
			pntsvector.push_back(ipnt);
		}

		std::cout<<"reading "<<pntsvector.size()<<" points...\n";
	}

	static void read_smesh_elements()
	{
		std::getline(inputfile,line);
		
		// get lines number
		int nelem;		// # of elements
		int bndry;	    // # of boundary markers

		std::sscanf(line.c_str(),"%d %d",&nelem, &bndry);

		std::vector<Model::mshElement>& elemsvector = Model::GetmshElementsVector();

		if(nelem==0){
			std::cout<<"ERROR: number of elements is = "<<nelem<<"\n";
			return;
		}

		//smesh_material_falg
		for( int i = 0; i < nelem; i++ )
		{
			std::getline(inputfile,line);
			Model::mshElement iele;
			int nnodes,n1,n2,n3,n4,bndry,matid(1.0);
			
			if (smesh_material_falg)
				std::sscanf(line.c_str(),"%d %d %d %d %d %d %d",&nnodes,&n1,&n2,&n3,&n4,&bndry,&matid);
			else
				std::sscanf(line.c_str(),"%d %d %d %d %d %d",&nnodes,&n1,&n2,&n3,&n4,&bndry);
			
			iele.points.clear();
			iele.points.push_back(n1);
			iele.points.push_back(n2);
			iele.points.push_back(n3);
			iele.points.push_back(n4);
			
			iele.bndry = bndry;
			iele.mat = matid;

			elemsvector.push_back(iele);
		}

		std::cout<<"reading "<<elemsvector.size()<<" elements...\n";
	}

	static void read_smesh2mpm()
	{	
		std::cout<<"reading SMESH file...\n";
		
		while (std::getline(inputfile,line))
		{
			// material flag
			if (line.find("SMESH.MATERIAL.FLAG")!=std::string::npos)
			{
				std::cout<<"assuming mesh with material ...\n";
				smesh_material_falg = true;
			}

			// points
			if (line.find("SMESH.POINTS")!=std::string::npos)
			{
				std::cout<<"reading points in SMESH file...\n";
				read_smesh_points();
			}

			// elements
			if (line.find("SMESH.ELEMENTS")!=std::string::npos)
			{	
				std::cout<<"reading elements in SMESH file...\n";
				read_smesh_elements();
				break;
			}
		}
	}

	static void init_file_name(int argc, char **argv)
	{
		// path set
		if(argc>0){
			std::string argv_str(argv[1]);
			std::string base = argv_str.substr(0, argv_str.find_last_of("/"));
			path = base;
		}

		// name of file to process
		if (argc > 1) {
			input_fname = argv[1];
		}
		else {
			std::cout<<"ERROR: please insert the file to process...\n";
			return;
	    }

		// name and path of output file
		out_fname = "/mpm.part";
		path_out_fname = path + out_fname;

	}

	static void read_grid()
	{
		std::cout<<"reading GRID data...\n";
		
		Vector3 lim_min,lim_max;
		Vector3 celldim;

		while (std::getline(inputfile,line))
		{
			// limits
			if (line.find("GRID.LIMITS")!=std::string::npos)
			{
				std::cout<<"reading grid limits...\n";
				std::getline(inputfile,line);
				std::sscanf(line.c_str(),"%lf%lf%lf%lf%lf%lf",&lim_min.x,&lim_min.y,&lim_min.z,&lim_max.x,&lim_max.y,&lim_max.z);				
			}

			// grid dimension
			if (line.find("GRID.DIMENSION")!=std::string::npos)
			{	
				std::cout<<"reading grid dimension...\n";
				std::getline(inputfile,line);
				std::sscanf(line.c_str(),"%lf%lf%lf",&celldim.x, &celldim.y, &celldim.z);
				break;
			}
		}

		// init the grid
		Model::initGrid(lim_min,lim_max,celldim);
	}

	//** globals

	void ReadInputFile(int argc, char **argv)
	{
		init_file_name(argc, argv);

		inputfile.open (input_fname.c_str(), std::ifstream::in);

		while (std::getline(inputfile,line))
		{
			// DEM to MPM
			if (line.find("DEM.TO.MPM")!=std::string::npos)
			{
				read_dem2mpm();
			}

			// total horizonts
			if (line.find("DEM.NUM.HORIZONT")!=std::string::npos)
			{
				read_dem_total_horizonts();
			}

			// read horizonts
			if (line.find("DEM.HORIZONT")!=std::string::npos)
			{
				read_dem_horizonts();
			}

			// SMESH to MPM
			if (line.find("SMESH")!=std::string::npos)
			{
				read_smesh2mpm();
			}

			// GRID
			if (line.find("GRID")!=std::string::npos)
			{
				read_grid();
			}

		}
		// close the file
		if(inputfile.is_open()) inputfile.close();
	}

	void WriteOutputFile()
	{
		using namespace std;
		outfile.open(path_out_fname.c_str(), fstream::out);
		
		// test
		vector<Model::Particle>& parvec = Model::GetParticleVector();

		// header for plot
		outfile<<"id x y z vol lp matid\n";
		outfile<<"%PARTICLES\n";
		outfile<<parvec.size()<<"\n";

		for (size_t i = 0; i < parvec.size(); ++i)
		{
			outfile<<parvec.at(i).id<<" ";
			outfile<<parvec.at(i).pos.x<<" ";
			outfile<<parvec.at(i).pos.y<<" ";
			outfile<<parvec.at(i).pos.z<<" ";
			outfile<<parvec.at(i).vol<<" ";
			outfile<<parvec.at(i).lp<<" ";
			outfile<<parvec.at(i).matid<<"\n";
		}

		cout<<"writing "<<parvec.size()<<" particles...\n";
		cout<<"please see the output file, "<<out_fname.c_str()<<"...\n";

		// close the file
		if(outfile.is_open()) outfile.close();

		// write the grid
		ParaView::writegrid();
		ParaView::writegridcell();
		ParaView::writeparticles();
	}

	std::string pathGet()
	{
		return path;
	}
}
