#include "ensemble.h"

ensemble::ensemble(int num, int gp, float lr, float hr, int fd, int as) {
	n = num;
	grace_period = gp;
	learning_rate = lr;
	hidden_ratio = hr;
	fp_dim = fd;
	ae_size = as;
	n_trained = 0;
	n_executed = 0;

	n_ae = fp_dim / ae_size;
	ensemble_layer = new autoencoder *[n_ae];
	parameter param1(ae_size, 0, learning_rate, 0, 0, hidden_ratio);
	for (int i = 0; i < n_ae; ++i)
		ensemble_layer[i] = new autoencoder(param1, "/home/pi/projects/csiauthenticator/model_file/el_" + std::to_string(i), true);

	parameter param2(n_ae, 0, learning_rate, 0, 0, hidden_ratio);
	output_layer = new autoencoder(param2, "/home/pi/projects/csiauthenticator/model_file/ol", false);
}

ensemble::~ensemble() {
	delete output_layer;
	for (int i = 0; i < n_ae; ++i)
		delete ensemble_layer[i];
	delete ensemble_layer;
}

void ensemble::normalize(double *x) {
	double sc_max = -10000, sc_min = 10000;
	for (int i = 0; i < fp_dim; ++i) {
		sc_max = std::max(sc_max, x[i]);
		sc_min = std::min(sc_min, x[i]);
	}
	double tmp = sc_max - sc_min + 0.0000000001;
	for (int i = 0; i < fp_dim; ++i) {
		// printf("%lf, ", x[i]);
		x[i] = (x[i] - sc_min) / tmp;
		// printf("%lf, ", x[i]);
	}
	// printf(" -- %lf -- %lf\n", sc_max, sc_min);
}

double ensemble::process(double *x) {
	normalize(x);
	double rmse = execute(x), tmp = 0;

	// 如果新样本合格（rmse < 0.10），则更新模型
	if (rmse <= 0.2)
		tmp = train(x);

	return rmse;
}

double ensemble::train(double *x) {
	normalize(x);
	double *output_l1 = new double[n_ae];
	double *xi = new double[ae_size];
	double rmse = 0;

	for (int i = 0; i < n_ae; ++i) {
		for (int j = 0; j < ae_size; ++j)
			xi[j] = x[ae_size * i + j];
		output_l1[i] = ensemble_layer[i]->train(xi);
		// std::cout << "ol: " << i << " ---- " << output_l1[i] << std::endl;
	}

	rmse = output_layer->train(output_l1);
	// std::cout << rmse << std::endl;

	delete[] xi;
	delete[] output_l1;
	n_trained++;

	// std::cout << rmse << std::endl;
	return rmse;
}

double ensemble::execute(double *x) {
	double *output_l1 = new double[n_ae];
	double *xi = new double[ae_size];
	double rmse = 0;

	for (int i = 0; i < n_ae; ++i) {
		for (int j = 0; j < ae_size; ++j) {
			xi[j] = x[ae_size * i + j];
			// printf("%lf, ", xi[j]);
		}
		// printf("\n");
		output_l1[i] = ensemble_layer[i]->execute(xi);
	}

	rmse = output_layer->execute(output_l1);

	delete[] xi;
	delete[] output_l1;
	n_executed++;

	return rmse;
}

void ensemble::load_model() {
	for (int i = 0; i < n_ae; ++i)
		ensemble_layer[i]->load_model();
	output_layer->load_model();
}

void ensemble::save_model() {
	for (int i = 0; i < n_ae; ++i)
		ensemble_layer[i]->save_model();
	output_layer->save_model();
}
