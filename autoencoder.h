#ifndef AUTOENCODER_H_
#define AUTOENCODER_H_

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <cstring>
#include <string>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>

class parameter {
	public:
		int visible;
		int hidden;
		float learning_rate;
		float corruption_level;
		int grace_period;
		float hidden_ratio;
		parameter(){}
		parameter(int, int, float, float, int, float);
};

class autoencoder {
	private:
		parameter param;
		double *norm_max;
		double *norm_min;
		double *hidden_bias;
		double *visible_bias;
		double **weight;
		std::string model_file;
		bool flag;
		int n;
	public:
		autoencoder(){}
		autoencoder(parameter, std::string, bool);
		~autoencoder();
		void normalize(double *);
		int load_model();
		int save_model();
		bool in_grace();
		void get_corrupted_input(double *, double *);
		void get_hidden_values(double *, double *);
		void get_reconstructed_input(double *, double *);
		double get_rmse(double *, double *);
		double train(double *);
		void reconstruct(double *, double *);
		double execute(double *);
};

#endif
