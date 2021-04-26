#include "autoencoder.h"
#include "utils.h"
#include <math.h>
#include <algorithm>
#include <random>
#include <ctime>
#include <cfloat>

using boost::property_tree::ptree;
using boost::property_tree::read_json;
using boost::property_tree::write_json;

parameter::parameter(int nv, int nh, float lr, float cl, int gp, float hr) {
	visible = nv;
	hidden = nh;
	learning_rate = lr;
	corruption_level = cl;
	grace_period = gp;
	hidden_ratio = hr;
}

autoencoder::autoencoder(parameter par, std::string mf, bool f) {
	param = par;
	model_file = mf;
	flag = f;

	if (param.hidden_ratio != 0)
		param.hidden = ceil((float)param.visible * param.hidden_ratio);

	norm_max = new double[param.visible];
	norm_min = new double[param.visible];
	for (int i = 0; i < param.visible; ++i) {
		norm_max[i] = -10000;
		norm_min[i] = 10000;
	}
	// std::memset(norm_max, -10000, param.visible);
	// std::memset(norm_min, 10000, param.visible);
	// std::cout << norm_max[param.visible - 2] << std::endl;
	// std::cout << norm_min[param.visible - 2] << std::endl;

	n = 0;

	std::default_random_engine e(time(0));
	double max = 1.0 / ((double)param.visible);
	std::uniform_real_distribution<double> uniform(-max, max);
	weight = new double *[param.visible];
	for (int i = 0; i < param.hidden; ++i) {
		weight[i] = new double[param.visible];
		for (int j = 0; j < param.visible; ++j) {
			weight[i][j] = uniform(e);
		}
	}

	hidden_bias = new double[param.hidden];
	visible_bias = new double[param.visible];
	memset(hidden_bias, 0, param.hidden);
	memset(visible_bias, 0, param.visible);
}

autoencoder::~autoencoder () {
	delete[] norm_max;
	delete[] norm_min;
	delete[] hidden_bias;
	delete[] visible_bias;
	for (int i = 0; i < param.hidden; ++i)
		delete[] weight[i];
	delete weight;
}

void autoencoder::normalize(double *x) {
	double sc_max = -10000, sc_min = 10000;
	for (int i = 0; i < param.visible; ++i) {
		sc_max = std::max(sc_max, x[i]);
		sc_min = std::min(sc_min, x[i]);
		// x[i] = (x[i] - norm_min[i]) / (norm_max[i] - norm_min[i] + 0.0000000001);
	}
	double tmp = sc_max - sc_min + 0.0000000001;
	for (int i = 0; i < param.visible; ++i) {
		// printf("%lf, ", x[i]);
		x[i] = (x[i] - sc_min) / tmp;
		// printf("%lf, ", x[i]);
	}
	// printf(" -- %lf -- %lf\n", sc_max, sc_min);
}

int autoencoder::load_model() {
	try {
		ptree pt;
		read_json(model_file, pt);

		// n = pt.get<int>("train_samples", 0);
		// n_visible = pt.get<int>("input_neuron", 0);
		// n_hidden = pt.get<int>("hidden_neuron", 0);

		int i = 0, j = 0;

		// load norm_max
		i = 0;
		BOOST_FOREACH(boost::property_tree::ptree::value_type& v, pt.get_child("norm_max")) {
			assert(v.first.empty());
			norm_max[i] = v.second.get_value<double>();
			i++;
		}

		// load norm_min
		i = 0;
		BOOST_FOREACH(boost::property_tree::ptree::value_type& v, pt.get_child("norm_min")) {
			assert(v.first.empty());
			norm_min[i] = v.second.get_value<double>();
			i++;
		}

		// load hidden_bias
		i = 0;
		BOOST_FOREACH(boost::property_tree::ptree::value_type& v, pt.get_child("hidden_bias")) {
			assert(v.first.empty());
			hidden_bias[i] = v.second.get_value<double>();
			i++;
		}

		// load visible_bias
		i = 0;
		BOOST_FOREACH(boost::property_tree::ptree::value_type& v, pt.get_child("visible_bias")) {
			assert(v.first.empty());
			visible_bias[i] = v.second.get_value<double>();
			i++;
		}

		// load weight
		i = 0;
		BOOST_FOREACH (boost::property_tree::ptree::value_type& row_pair, pt.get_child("weight")) {
			assert(row_pair.first.empty());
			j = 0;
			BOOST_FOREACH (boost::property_tree::ptree::value_type& item_pair, row_pair.second) {
				assert(item_pair.first.empty());
				weight[i][j] = item_pair.second.get_value<double>();
				j++;
			}
			i++;
		}
	} catch (std::exception const& e) {
		std::cerr << e.what() << std::endl;
		return -1;
	}

	return 0;
}

int autoencoder::save_model() {
	try {
		ptree pt;
		pt.put("model_type", "AutoEncoder");
		pt.put("model_version", "1.0");
		// pt.put("train_samples", n);
		// pt.put("input_neuron", visible);
		// pt.put("hidden_neuron", hidden);

		// save norm_max
		ptree nmax;
		ptree array_element_nmax;
		for(int i = 0; i < param.visible; ++i) {
			array_element_nmax.put_value(norm_max[i]);
			nmax.push_back(std::make_pair("", array_element_nmax));
		}
		pt.add_child("norm_max", nmax);

		// save norm_min
		ptree nmin;
		ptree array_element_nmin;
		for(int i = 0; i < param.visible; ++i) {
			array_element_nmin.put_value(norm_min[i]);
			nmin.push_back(std::make_pair("", array_element_nmin));
		}
		pt.add_child("norm_min", nmin);

		// save hidden_bias
		ptree children_hbias;
		ptree array_element_hbias;
		for(int i = 0; i < param.hidden; ++i) {
			array_element_hbias.put_value(hidden_bias[i]);
			children_hbias.push_back(std::make_pair("", array_element_hbias));
		}
		pt.add_child("hidden_bias", children_hbias);

		// save visible_bias
		ptree children_vbias;
		ptree array_element_vbias;
		for(int i = 0; i < param.visible; ++i) {
			array_element_vbias.put_value(visible_bias[i]);
			children_vbias.push_back(std::make_pair("", array_element_vbias));
		}
		pt.add_child("visible_bias", children_vbias);

		// save weight
		ptree children_W;
		for(int i = 0; i < param.hidden; ++i) {
			ptree children_hidden_w;
			ptree array_element_w;
			for(int j = 0; j < param.visible; ++j) {
				array_element_w.put_value(weight[i][j]);
				children_hidden_w.push_back(std::make_pair("", array_element_w));
			}
			children_W.push_back(std::make_pair("", children_hidden_w));
		}
		pt.add_child("weight", children_W);

		// write json
		write_json(model_file, pt);
	} catch (std::exception const& e) {
		std::cerr << e.what() << std::endl;
		return -1;
	}

	return 0;
}
bool autoencoder::in_grace() {
	return n < param.grace_period;
}

void autoencoder::get_corrupted_input(double *x, double *tilde_x) {
	std::default_random_engine e(time(0));
	std::binomial_distribution<int> binomial(1, 1 - param.corruption_level);
	for (int i = 0; i < param.visible; ++i) {
		if (x[i] == 0)
			tilde_x[i] = 0;
		else
			tilde_x[i] = binomial(e);
	}
}

void autoencoder::get_hidden_values(double *x, double *y) {
	for (int i = 0; i < param.hidden; ++i) {
		y[i] = 0;
		for (int j = 0; j < param.visible; ++j)
			y[i] += weight[i][j] * x[j];
		y[i] += hidden_bias[i];
		y[i] = sigmoid(y[i]);
	}
}

void autoencoder::get_reconstructed_input(double *y, double *z) {
	for (int i = 0; i < param.visible; ++i) {
		z[i] = 0;
		for (int j = 0; j < param.hidden; ++j)
			z[i] += weight[j][i] * y[j];
		z[i] += visible_bias[i];
		z[i] = sigmoid(z[i]);
	}
}

double autoencoder::get_rmse(double *x, double *z) {
	double rmse = 0;
	for (int i = 0; i < param.visible; ++i)
		rmse += (x[i] - z[i]) * (x[i] - z[i]);
	rmse /= param.visible;
	rmse = sqrt(rmse);

	return rmse;
}

double autoencoder::train(double *x) {
	n++;

	for (int i = 0; i < param.visible; ++i) {
		norm_max[i] = std::max(norm_max[i], x[i]);
		norm_min[i] = std::min(norm_min[i], x[i]);
	}
	// if (flag)
	// 	normalize(x);
	// else
	// 	for (int i = 0; i < param.visible; ++i)
	// 		xi] = (x[i] - norm_min[i]) / (norm_max[i] - norm_min[i] + 0.0000000001);

	double *tilde_x = new double[param.visible];
	if (param.corruption_level > 0.0)
		get_corrupted_input(x, tilde_x);
	else
		for (int i = 0; i < param.visible; ++i)
			tilde_x[i] = x[i];

	double *y = new double[param.hidden];
	double *z = new double[param.visible];
	get_hidden_values(tilde_x, y);
	get_reconstructed_input(y, z);

	double *visible_error = new double[param.visible];
	double *hidden_error = new double[param.hidden];

	for (int i = 0; i < param.visible; ++i) {
		visible_error[i] = x[i] - z[i];
		visible_bias[i] += param.learning_rate * visible_error[i] / param.visible;
	}

	for (int i = 0; i < param.hidden; ++i) {
		hidden_error[i] = 0;
		for (int j = 0; j < param.visible; ++j) {
			hidden_error[i] += weight[i][j] * visible_error[j];
		}
		hidden_error[i] = hidden_error[i] * y[i] * (1 - y[i]);
		hidden_bias[i] += param.learning_rate * hidden_error[i] / param.hidden;
	}

	for (int i = 0; i < param.hidden; ++i)
		for (int j = 0; j < param.visible; ++j)
			weight[i][j] += param.learning_rate * (hidden_error[i] * tilde_x[j] + visible_error[j] * y[i]) / n;

	delete[] hidden_error;
	delete[] visible_error;
	delete[] z;
	delete[] y;
	delete[] tilde_x;

	return get_rmse(x, z);
}

void autoencoder::reconstruct(double *x, double *z) {
	double *y = new double[param.hidden];
	get_hidden_values(x, y);
	get_reconstructed_input(y, z);
	delete[] y;
}

double autoencoder::execute(double *x) {
	if (n < param.grace_period)
		return 0.0;
	else {
		// if (flag)
		// 	normalize(x);
		// else {
		if (!flag) {
			double rmse = 0;
			for (int i = 0; i < param.visible; ++i) {
				printf("%lf, ", x[i]);
				rmse += x[i];
				// x[i] = (x[i] - norm_min[i]) / (norm_max[i] - norm_min[i] + 0.0000000001);
				// printf("%lf, ", x[i]);
			}
			printf("\n");
			return rmse / param.visible;
		}
		double *z = new double[param.visible];
		reconstruct(x, z);
		return get_rmse(x, z);
	}
}
