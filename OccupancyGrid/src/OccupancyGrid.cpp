/**
 *      @file OccupancyGrid.cpp
 *      @date May 23, 2012
 *      @author Brian Peasley
 *      @author Frank Dellaert
 */

#include "../include/OccupancyGrid.h"


///constructor
///Creates a 2d grid of cells with the origin in the center of the grid
OccupancyGrid::OccupancyGrid(double width, double height, double resolution){
	width_ 		= 	width/resolution;
	height_ 	= 	height/resolution;
	res_		=	resolution;

	for(Index i = 0; i < cellCount(); i++){
		cells_.push_back(i);
		heat_map_.push_back(1);
	}
}

/// Returns an empty occupancy grid of size width_ x height_
OccupancyGrid::Occupancy OccupancyGrid::emptyOccupancy() const {
	Occupancy		occupancy;		//mapping from Index to value (0 or 1)
	for(size_t i = 0; i < cellCount(); i++)
		occupancy.insert(pair<Index, size_t>((Index)i,0));

	return occupancy;
}

///add a prior
void OccupancyGrid::addPrior(Index cell, double prior){
	size_t numStates = 2;
	DiscreteKey key(cell, numStates);

	//add a factor
	vector<double> table(2);
	table[0] = 1-prior;
	table[1] = prior;
	add(key, table);
}

///add a laser measurement
void OccupancyGrid::addLaser(const Pose2 &pose, double range){
	//ray trace from pose to range t//a >= 1 accept new stateo find all cells the laser passes through
	double x = pose.x();		//start position of the laser
	double y = pose.y();
	double step = res_/8.0;	//amount to step in each iteration of laser traversal

	Index 			key;
	vector<Index> 	cells;		//list of keys of cells hit by the laser

	//traverse laser
	for(double i = 0; i < range; i += step){
		//get point on laser
		x = pose.x() + i*cos(pose.theta());
		y = pose.y() + i*sin(pose.theta());

		//printf("%lf %lf\n", x, y);
		//get the key of the cell that holds point (x,y)
		key = keyLookup(x,y);

		//add cell to list of cells if it is new
		if((i == 0 || key != cells[cells.size()-1]) && key < cells_.size()-1){
			cells.push_back(key);
		}
	}

	//last cell hit by laser has higher probability to be flipped
	if(key < cells_.size()-1)
		heat_map_[key] = 4;

	if(cells.size() > 0){

		//add a factor that connects all those cells
		laser_indices_.push_back(factors_.size());
		push_back(boost::make_shared<LaserFactor>(cells));

		pose_.push_back(pose);
		range_.push_back(range);
	}

}
/// returns the key of the cell in which point (x,y) lies.
Index OccupancyGrid::keyLookup(double x, double y) const {
	//move (x,y) to the nearest resolution
	x *= (1.0/res_);
	y *= (1.0/res_);

	//round to nearest integer
	x = (double)((int)x);
	y = (double)((int)y);

	//determine index
	x += width_/2;
	y = height_/2 - y;

	//bounds checking
	size_t index = y*width_ + x;
	index = index >= width_*height_ ? -1 : index;

	return cells_[index];
}

/**
 * @brief Computes the value of a laser factor
 * @param index defines which laser is to be used
 * @param occupancy defines the grid which the laser will be evaulated with
 * @ret a double value that is the value of the specified laser factor for the grid
 */

/// returns the sum of the laser factors for the current state of the grid
double OccupancyGrid::operator()(const Occupancy &occupancy) const {
	double value = 0;

	// loop over all laser factors in the graph
	//printf("%ld\n", (*this).size());

	for(Index i = 0; i < laser_indices_.size(); i++){
		value += laserFactorValue(i, occupancy);
	}

	return value;
}

void OccupancyGrid::saveLaser(const char *fname) const {
	FILE *fptr = fopen(fname, "w");
	Occupancy occupancy;
	for(Index i = 0; i < pose_.size(); i++){
		fprintf(fptr, "%lf %lf %lf %lf\n", pose_[i].x(), pose_[i].y(), pose_[i].theta(), range_[i]);
	}
	fclose(fptr);
}

void OccupancyGrid::saveHeatMap(const char *fname) const {
	FILE *fptr = fopen(fname, "wb");
	fprintf(fptr, "P3 %d %d 255\n", (int)width_, (int)height_);


	unsigned int red, green, blue;
	unsigned int byte;
	for(size_t it = 0; it < cellCount(); it++){
		byte = 0;
		if(heat_map_[it] == 4)
			fprintf(fptr, "255 0 0\n");
		else
			fprintf(fptr, "0 0 255\n");
	}
	fclose(fptr);

}

///* ************************************************************************* */
//TEST_UNSAFE( OccupancyGrid, Test1) {
//	//Build a small grid and test optimization
//
//	//Build small grid
//	double width 		=	20; 		//meters
//	double height 		= 	20; 		//meters
//	double resolution 	= 	0.2; 	//meters
//	OccupancyGrid occupancyGrid(width, height, resolution); //default center to middle
//
//	//Add measurements
////	Pose2 pose(0,0,0);
////	double range = 4.499765;
////
////	occupancyGrid.addPrior(0, 0.7);
////	EXPECT_LONGS_EQUAL(1, occupancyGrid.size());
////
////	occupancyGrid.addLaser(pose, range);
////	EXPECT_LONGS_EQUAL(2, occupancyGrid.size());
//
//	//add lasers
//	int n_frames = 1;
//	int n_lasers_per_frame = 640;
//	char laser_list_file[1000];
//
//
//	for(int i = 0; i < n_frames; i++){
//		sprintf(laser_list_file, "/home/brian/Desktop/research/user/bpeasle/code/KinectInterface/Data/ScanLinesAsLasers/KinectRecording9/laser_list%.4d", i);
//		FILE *fptr = fopen(laser_list_file,"r");
//		double x,y, theta;
//		double range, angle;
//		fscanf(fptr, "%lf %lf %lf", &x, &y, &theta);
//
//		for(int j = 0; j < n_lasers_per_frame; j++){
//			fscanf(fptr, "%lf %lf", &range, &angle);
//			//if(j == 159){
//				Pose2 pose(x,y, theta+angle);
//
//				occupancyGrid.addLaser(pose, range);
//			//}
//		}
//		fclose(fptr);
//
//	}
//
//
////	OccupancyGrid::Occupancy occupancy = occupancyGrid.emptyOccupancy();
////	EXPECT_LONGS_EQUAL(900, occupancyGrid.laserFactorValue(0,occupancy));
////
////
////	occupancy[16] = 1;
////	EXPECT_LONGS_EQUAL(1, occupancyGrid.laserFactorValue(0,occupancy));
////
////	occupancy[15] = 1;
////	EXPECT_LONGS_EQUAL(1000, occupancyGrid.laserFactorValue(0,occupancy));
////
////	occupancy[16] = 0;
////	EXPECT_LONGS_EQUAL(1000, occupancyGrid.laserFactorValue(0,occupancy));
//
//
//	//run MCMC
//	OccupancyGrid::Marginals occupancyMarginals = occupancyGrid.runMetropolis(50000);
//	//EXPECT_LONGS_EQUAL( (width*height)/pow(resolution,2), occupancyMarginals.size());
//	//select a cell at a random to flip
//
//
//	printf("\n");
//	for(size_t i = 0, it = 0; i < occupancyGrid.height(); i++){
//		for(size_t j = 0; j < occupancyGrid.width(); j++, it++){
//			printf("%.2lf ", occupancyMarginals[it]);
//		}
//		printf("\n");
//	}
//
//	char marginalsOutput[1000];
//	sprintf(marginalsOutput, "/home/brian/Desktop/research/user/bpeasle/code/KinectInterface/marginals.txt");
//	FILE *fptr = fopen(marginalsOutput, "w");
//	fprintf(fptr, "%d %d\n", occupancyGrid.width(), occupancyGrid.height());
//
//	for(int i = 0; i < occupancyMarginals.size(); i++){
//		fprintf(fptr, "%lf ", occupancyMarginals[i]);
//	}
//	fclose(fptr);
//
//}
