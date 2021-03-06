#include "stdafx.h" 
#include <random>
#include <direct.h>
#include <opencv2/opencv.hpp>
#include <vector>

#define VERBOSE false

using namespace cv;

// Barebones version of OpenCV's distance computer
// in order to take advantage of the parallel loop
class KMeansDistanceComputer : public ParallelLoopBody
{
public:
	KMeansDistanceComputer(double *distances_,
		int *labels_,
		const Mat& data_,
		const Mat& centers_)
		: distances(distances_),
		labels(labels_),
		data(data_),
		centers(centers_)
	{
	}

	void operator()(const Range& range) const
	{
		const int begin = range.start;
		const int end = range.end;
		const int K = centers.rows;
		const int dims = centers.cols;

		for (int i = begin; i < end; ++i)
		{
			const float *sample = data.ptr<float>(i);
			int k_best = 0;
			double min_dist = DBL_MAX;

			for (int k = 0; k < K; k++)
			{
				const float* center = centers.ptr<float>(k);
				const double dist = sqrt(pow(sample[0] - center[0], 2.0) + pow(sample[1] - center[1], 2.0) + pow(sample[2] - center[2], 2.0));

				if (min_dist > dist)
				{
					min_dist = dist;
					k_best = k;
				}
			}

			distances[i] = min_dist;
			labels[i] = k_best;
		}
	}

private:
	double *distances;
	int *labels;
	const Mat& data;
	const Mat& centers;
};

int main(int argc, char** argv)
{
	// Parse input arguments
	cv::String filename = "assets/test.png";
	int kGroups = 6;
	int numIterations = 300;
	int bpm = 95;
	int clusterAnims = 2;
	float flowSpeed = 0.06f;

	int animationTime = std::round(25.0f * 60.0f / bpm); // Assume 25 fps output

	// Load input image
	Mat image = imread(filename, IMREAD_COLOR);
	if (!image.data)
	{
		std::cout << "Could not find the specified image" << std::endl;
		return -1;
	}
	std::cout << "Image loaded" << std::endl;

	// Create directory to save images
	if (_mkdir("Results") != 0 && errno != EEXIST)
	{
		std::cout << "Could not make results directory" << std::endl;
		return -1;
	}

	// Matrix of every pixel in it's own row
	Mat pixels(image.rows * image.cols, 3, CV_32F);
	for (int y = 0; y < image.rows; y++)
	{
		for (int x = 0; x < image.cols; x++)
		{
			for (int z = 0; z < 3; z++)
			{
				pixels.at<float>(y + x * image.rows, z) = image.at<Vec3b>(y, x)[z];
			}
		}
	}

	// Setup
	const int N = pixels.rows;
	const int dims = 3;
	const int type = pixels.depth();
	RNG& rng = theRNG();

	Mat labels;
	labels.create(cv::Size(1,N), CV_32S);
	int* labelsPtr = labels.ptr<int>();

	Mat centers(kGroups, dims, type), old_centers(kGroups, dims, type), animations(kGroups, dims, type);

	AutoBuffer<int, 64> counters(kGroups);
	AutoBuffer<double, 64> dists(N);

	// Kmeans
	for (int frame = 0; frame < numIterations + 1; frame++)
	{
		swap(centers, old_centers);

		if (frame == 0)
		{
			// First iteration, generate random clusters centers
			for (int k = 0; k < kGroups; k++)
			{
				float* center = centers.ptr<float>(k);
				for (int j = 0; j < dims; j++)
				{
					center[j] = (float)rng*255.0f;
				}

				// Set up animation map, if first component is -1
				// then no current animation is being interpolated
				float* animation = animations.ptr<float>(k);
				animation[0] = -1;
			}
		}
		else
		{
			// Compute the centers of clusters based on assigned labels
			centers = Scalar(0);
			for (int k = 0; k < kGroups; k++)
			{
				counters[k] = 0;
			}

			// Sum cluster values
			for (int i = 0; i < N; i++)
			{
				const float* sample = pixels.ptr<float>(i);
				int k = labelsPtr[i];
				float* center = centers.ptr<float>(k);
				for (int j = 0; j < dims; j++)
					center[j] += sample[j];
				counters[k]++;
			}

			// Check for new animation calculations
			if (frame % animationTime == 1)
			{
				// Clear out old animations
				std::vector<int> clustersToAnimate;
				for (int k = 0; k < kGroups; k++)
				{
					float* animation = animations.ptr<float>(k);
					animation[0] = -1;

					clustersToAnimate.push_back(k);
				}

				std::random_device rd;
				std::mt19937 g(rd());
				std::shuffle(clustersToAnimate.begin(), clustersToAnimate.end(), g);

				for (int i = 0; i < clusterAnims; i++)
				{
					float* animation = animations.ptr<float>(clustersToAnimate[i]);
					for (int j = 0; j < dims; j++)
					{
						animation[j] = (float)rng*255.0f;
					}
				}
			}

			// Take average
			for (int k = 0; k < kGroups; k++)
			{
				float* center = centers.ptr<float>(k);
				float* old_center = old_centers.ptr<float>(k);
				float* animation = animations.ptr<float>(k);
				for (int j = 0; j < dims; j++)
				{
					// Take a step to computed center, or if animated, to the random animated cluster
					float speed = pow(flowSpeed, 1.0f + 1.0f * (frame % animationTime) / animationTime);
					if (frame == 1)
					{
						// Take a big step on first frame to mitigate the randomness of first clusters
						if (counters[k] == 0)
						{
							center[j] = old_center[j];
						}
						else
						{
							center[j] /= (float)counters[k];
							center[j] = old_center[j] + (center[j] - old_center[j]);
						}
					}
					else if (animation[0] == -1)
					{
						if (counters[k] == 0)
						{
							center[j] = old_center[j];
						}
						else
						{
							center[j] /= (float)counters[k];
							center[j] = old_center[j] + (center[j] - old_center[j]) * speed;
						}
					}
					else
					{
						center[j] = old_center[j] + (animation[j] - old_center[j]) * speed * 2.0;
					}
				}
			}
		}

		// Assign labels
		cv::parallel_for_(Range(0, N), KMeansDistanceComputer(dists.data(), labelsPtr, pixels, centers), (double)divUp((size_t)(dims * N * kGroups), 1000.0));

		// Don't save the first iteration, since the clusters were randomly generated
		if (frame != 0)
		{
			// Create new image
			Mat new_image(image.size(), image.type());
			for (int y = 0; y < image.rows; y++)
			{
				for (int x = 0; x < image.cols; x++)
				{
					int cluster_idx = labels.at<int>(y + x * image.rows, 0);
					new_image.at<Vec3b>(y, x)[0] = centers.at<float>(cluster_idx, 0);
					new_image.at<Vec3b>(y, x)[1] = centers.at<float>(cluster_idx, 1);
					new_image.at<Vec3b>(y, x)[2] = centers.at<float>(cluster_idx, 2);
				}
			}

			// Save frame
			cv::String filename("Results\\frame" + std::to_string(frame) + ".png");
			cv::imwrite(filename, new_image);

			std::cout << "Frame " + std::to_string(frame) + " completed" << std::endl;
		}
	}

    return 0;
}
