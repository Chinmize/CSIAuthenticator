#ifndef ENSEMBLE_H_
#define ENSEMBLE_H_

#include "autoencoder.h"

class ensemble {
	private:
		int n;
		int grace_period;
		float learning_rate;
		float hidden_ratio;
		int n_trained;
		int n_executed;
		int n_ae;
		int fp_dim;
		int ae_size;
		autoencoder **ensemble_layer;
		autoencoder *output_layer;
	public:
		ensemble(){}
		ensemble(int, int, float, float, int, int);
		~ensemble();
		void normalize(double *);
		double process(double *);
		double train(double *);
		double execute(double *);
		void load_model();
		void save_model();
};

#endif
