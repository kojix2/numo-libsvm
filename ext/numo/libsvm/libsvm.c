/**
 * LIBSVM interface for Numo::NArray
 */
#include "libsvm.h"

VALUE mNumo;
VALUE mLibsvm;
VALUE mSvm;

void print_null(const char *s) {}

/**
 * Train the SVM model according to the given training data.
 *
 * @overload train(x, y, param) -> Hash
 *
 * @param x [Numo::DFloat] (shape: [n_samples, n_features]) The samples to be used for training the model.
 * @param y [Numo::DFloat] (shape: [n_samples]) The labels or target values for samples.
 * @param param [Hash] The parameters of an SVM model.
 * @return [Hash] The model obtained from the training procedure.
 */
static
VALUE train(VALUE self, VALUE x_val, VALUE y_val, VALUE param_hash)
{
  struct svm_problem* problem;
  struct svm_parameter* param;
  narray_t* x_nary;
  narray_t* y_nary;
  double* x_pt;
  double* y_pt;
  int i, j;
  int n_samples;
  int n_features;
  struct svm_model* model;
  VALUE model_hash;

  /* Obtain C data structures. */
  if (CLASS_OF(x_val) != numo_cDFloat) {
    x_val = rb_funcall(numo_cDFloat, rb_intern("cast"), 1, x_val);
  }
  if (CLASS_OF(y_val) != numo_cDFloat) {
    y_val = rb_funcall(numo_cDFloat, rb_intern("cast"), 1, y_val);
  }
  if (!RTEST(nary_check_contiguous(x_val))) {
    x_val = nary_dup(x_val);
  }
  if (!RTEST(nary_check_contiguous(y_val))) {
    y_val = nary_dup(y_val);
  }
  GetNArray(x_val, x_nary);
  GetNArray(y_val, y_nary);
  param = rb_hash_to_svm_parameter(param_hash);

  /* Initialize some variables. */
  n_samples = (int)NA_SHAPE(x_nary)[0];
  n_features = (int)NA_SHAPE(x_nary)[1];
  x_pt = (double*)na_get_pointer_for_read(x_val);
  y_pt = (double*)na_get_pointer_for_read(y_val);

  /* Prepare LIBSVM problem. */
  problem = ALLOC(struct svm_problem);
  problem->l = n_samples;
  problem->x = ALLOC_N(struct svm_node*, n_samples);
  problem->y = ALLOC_N(double, n_samples);
  for (i = 0; i < n_samples; i++) {
    problem->x[i] = ALLOC_N(struct svm_node, n_features + 1);
    for (j = 0; j < n_features; j++) {
      problem->x[i][j].index = j + 1;
      problem->x[i][j].value = x_pt[i * n_features + j];
    }
    problem->x[i][n_features].index = -1;
    problem->x[i][n_features].value = 0.0;
    problem->y[i] = y_pt[i];
  }

  /* Perform training. */
  svm_set_print_string_function(print_null);
  model = svm_train(problem, param);
  model_hash = svm_model_to_rb_hash(model);
  svm_free_and_destroy_model(&model);

  for (i = 0; i < n_samples; xfree(problem->x[i++]));
  xfree(problem->x);
  xfree(problem->y);
  xfree(problem);
  xfree_svm_parameter(param);

  return model_hash;
}

/**
 * Perform cross validation under given parameters. The given samples are separated to n_fols folds.
 * The predicted labels or values in the validation process are returned.
 *
 * @overload cv(x, y, param, n_folds) -> Numo::DFloat
 *
 * @param x [Numo::DFloat] (shape: [n_samples, n_features]) The samples to be used for training the model.
 * @param y [Numo::DFloat] (shape: [n_samples]) The labels or target values for samples.
 * @param param [Hash] The parameters of an SVM model.
 * @param n_folds [Integer] The number of folds.
 * @return [Numo::DFloat] (shape: [n_samples]) The predicted class label or value of each sample.
 */
static
VALUE cross_validation(VALUE self, VALUE x_val, VALUE y_val, VALUE param_hash, VALUE nr_folds)
{
  const int n_folds = NUM2INT(nr_folds);
  struct svm_problem* problem;
  struct svm_parameter* param;
  narray_t* x_nary;
  narray_t* y_nary;
  double* x_pt;
  double* y_pt;
  int i, j;
  int n_samples;
  int n_features;
  size_t t_shape[1];
  VALUE t_val;
  double* t_pt;

  /* Obtain C data structures. */
  if (CLASS_OF(x_val) != numo_cDFloat) {
    x_val = rb_funcall(numo_cDFloat, rb_intern("cast"), 1, x_val);
  }
  if (CLASS_OF(y_val) != numo_cDFloat) {
    y_val = rb_funcall(numo_cDFloat, rb_intern("cast"), 1, y_val);
  }
  if (!RTEST(nary_check_contiguous(x_val))) {
    x_val = nary_dup(x_val);
  }
  if (!RTEST(nary_check_contiguous(y_val))) {
    y_val = nary_dup(y_val);
  }
  GetNArray(x_val, x_nary);
  GetNArray(y_val, y_nary);
  param = rb_hash_to_svm_parameter(param_hash);

  /* Initialize some variables. */
  n_samples = (int)NA_SHAPE(x_nary)[0];
  n_features = (int)NA_SHAPE(x_nary)[1];
  x_pt = (double*)na_get_pointer_for_read(x_val);
  y_pt = (double*)na_get_pointer_for_read(y_val);

  /* Prepare LIBSVM problem. */
  problem = ALLOC(struct svm_problem);
  problem->l = n_samples;
  problem->x = ALLOC_N(struct svm_node*, n_samples);
  problem->y = ALLOC_N(double, n_samples);
  for (i = 0; i < n_samples; i++) {
    problem->x[i] = ALLOC_N(struct svm_node, n_features + 1);
    for (j = 0; j < n_features; j++) {
      problem->x[i][j].index = j + 1;
      problem->x[i][j].value = x_pt[i * n_features + j];
    }
    problem->x[i][n_features].index = -1;
    problem->x[i][n_features].value = 0.0;
    problem->y[i] = y_pt[i];
  }

  /* Perform cross validation. */
  t_shape[0] = n_samples;
  t_val = rb_narray_new(numo_cDFloat, 1, t_shape);
  t_pt = (double*)na_get_pointer_for_write(t_val);
  svm_set_print_string_function(print_null);
  svm_cross_validation(problem, param, n_folds, t_pt);

  for (i = 0; i < n_samples; xfree(problem->x[i++]));
  xfree(problem->x);
  xfree(problem->y);
  xfree(problem);
  xfree_svm_parameter(param);

  return t_val;
}

/**
 * Predict class labels or values for given samples.
 *
 * @overload predict(x, param, model) -> Numo::DFloat
 *
 * @param x [Numo::DFloat] (shape: [n_samples, n_features]) The samples to calculate the scores.
 * @param param [Hash] The parameters of the trained SVM model.
 * @param model [Hash] The model obtained from the training procedure.
 * @return [Numo::DFloat] (shape: [n_samples]) The predicted class label or value of each sample.
 */
static
VALUE predict(VALUE self, VALUE x_val, VALUE param_hash, VALUE model_hash)
{
  struct svm_parameter* param;
  struct svm_model* model;
  struct svm_node* x_nodes;
  narray_t* x_nary;
  double* x_pt;
  size_t y_shape[1];
  VALUE y_val;
  double* y_pt;
  int i, j;
  int n_samples;
  int n_features;

  /* Obtain C data structures. */
  if (CLASS_OF(x_val) != numo_cDFloat) {
    x_val = rb_funcall(numo_cDFloat, rb_intern("cast"), 1, x_val);
  }
  if (!RTEST(nary_check_contiguous(x_val))) {
    x_val = nary_dup(x_val);
  }
  GetNArray(x_val, x_nary);
  param = rb_hash_to_svm_parameter(param_hash);
  model = rb_hash_to_svm_model(model_hash);
  model->param = *param;

  /* Initialize some variables. */
  n_samples = (int)NA_SHAPE(x_nary)[0];
  n_features = (int)NA_SHAPE(x_nary)[1];
  y_shape[0] = n_samples;
  y_val = rb_narray_new(numo_cDFloat, 1, y_shape);
  y_pt = (double*)na_get_pointer_for_write(y_val);
  x_pt = (double*)na_get_pointer_for_read(x_val);

  /* Predict values. */
  x_nodes = ALLOC_N(struct svm_node, n_features + 1);
  x_nodes[n_features].index = -1;
  x_nodes[n_features].value = 0.0;
  for (i = 0; i < n_samples; i++) {
    for (j = 0; j < n_features; j++) {
      x_nodes[j].index = j + 1;
      x_nodes[j].value = (double)x_pt[i * n_features + j];
    }
    y_pt[i] = svm_predict(model, x_nodes);
  }

  xfree(x_nodes);
  xfree_svm_model(model);
  xfree_svm_parameter(param);

  return y_val;
}

/**
 * Calculate confidence score for given samples.
 *
 * @overload decision_function(x, param, model) -> Numo::DFloat
 *
 * @param x [Numo::DFloat] (shape: [n_samples, n_features]) The samples to calculate the scores.
 * @param param [Hash] The parameters of the trained SVM model.
 * @param model [Hash] The model obtained from the training procedure.
 * @return [Numo::DFloat] (shape: [n_samples, n_classes]) Confidence score for each sample.
 */
static
VALUE decision_function(VALUE self, VALUE x_val, VALUE param_hash, VALUE model_hash)
{
  struct svm_parameter* param;
  struct svm_model* model;
  struct svm_node* x_nodes;
  narray_t* x_nary;
  double* x_pt;
  size_t y_shape[2];
  VALUE y_val;
  double* y_pt;
  double* dec_values;
  int y_cols;
  int i, j;
  int n_samples;
  int n_features;

  /* Obtain C data structures. */
  if (CLASS_OF(x_val) != numo_cDFloat) {
    x_val = rb_funcall(numo_cDFloat, rb_intern("cast"), 1, x_val);
  }
  if (!RTEST(nary_check_contiguous(x_val))) {
    x_val = nary_dup(x_val);
  }
  GetNArray(x_val, x_nary);
  param = rb_hash_to_svm_parameter(param_hash);
  model = rb_hash_to_svm_model(model_hash);
  model->param = *param;

  /* Initialize some variables. */
  n_samples = (int)NA_SHAPE(x_nary)[0];
  n_features = (int)NA_SHAPE(x_nary)[1];

  if (model->param.svm_type == ONE_CLASS || model->param.svm_type == EPSILON_SVR || model->param.svm_type == NU_SVR) {
    y_shape[0] = n_samples;
    y_shape[1] = 1;
    y_val = rb_narray_new(numo_cDFloat, 1, y_shape);
  } else {
    y_shape[0] = n_samples;
    y_shape[1] = model->nr_class * (model->nr_class - 1) / 2;
    y_val = rb_narray_new(numo_cDFloat, 2, y_shape);
  }

  x_pt = (double*)na_get_pointer_for_read(x_val);
  y_pt = (double*)na_get_pointer_for_write(y_val);

  /* Predict values. */
  if (model->param.svm_type == ONE_CLASS || model->param.svm_type == EPSILON_SVR || model->param.svm_type == NU_SVR) {
    x_nodes = ALLOC_N(struct svm_node, n_features + 1);
    x_nodes[n_features].index = -1;
    x_nodes[n_features].value = 0.0;
    for (i = 0; i < n_samples; i++) {
      for (j = 0; j < n_features; j++) {
        x_nodes[j].index = j + 1;
        x_nodes[j].value = (double)x_pt[i * n_features + j];
      }
      svm_predict_values(model, x_nodes, &y_pt[i]);
    }
    xfree(x_nodes);
  } else {
    y_cols = (int)y_shape[1];
    dec_values = ALLOC_N(double, y_cols);
    x_nodes = ALLOC_N(struct svm_node, n_features + 1);
    x_nodes[n_features].index = -1;
    x_nodes[n_features].value = 0.0;
    for (i = 0; i < n_samples; i++) {
      for (j = 0; j < n_features; j++) {
        x_nodes[j].index = j + 1;
        x_nodes[j].value = (double)x_pt[i * n_features + j];
      }
      svm_predict_values(model, x_nodes, dec_values);
      for (j = 0; j < y_cols; j++) {
        y_pt[i * y_cols + j] = dec_values[j];
      }
    }
    xfree(x_nodes);
    xfree(dec_values);
  }

  xfree_svm_model(model);
  xfree_svm_parameter(param);

  return y_val;
}

/**
 * Predict class probability for given samples. The model must have probability information calcualted in training procedure.
 * The parameter ':probability' set to 1 in training procedure.
 *
 * @overload predict_proba(x, param, model) -> Numo::DFloat
 *
 * @param x [Numo::DFloat] (shape: [n_samples, n_features]) The samples to predict the class probabilities.
 * @param param [Hash] The parameters of the trained SVM model.
 * @param model [Hash] The model obtained from the training procedure.
 * @return [Numo::DFloat] (shape: [n_samples, n_classes]) Predicted probablity of each class per sample.
 */
static
VALUE predict_proba(VALUE self, VALUE x_val, VALUE param_hash, VALUE model_hash)
{
  struct svm_parameter* param;
  struct svm_model* model;
  struct svm_node* x_nodes;
  narray_t* x_nary;
  double* x_pt;
  size_t y_shape[2];
  VALUE y_val = Qnil;
  double* y_pt;
  double* probs;
  int i, j;
  int n_samples;
  int n_features;

  param = rb_hash_to_svm_parameter(param_hash);
  model = rb_hash_to_svm_model(model_hash);
  model->param = *param;

  if ((model->param.svm_type == C_SVC || model->param.svm_type == NU_SVC) && model->probA != NULL && model->probB != NULL) {
    /* Obtain C data structures. */
    if (CLASS_OF(x_val) != numo_cDFloat) {
      x_val = rb_funcall(numo_cDFloat, rb_intern("cast"), 1, x_val);
    }
    if (!RTEST(nary_check_contiguous(x_val))) {
      x_val = nary_dup(x_val);
    }
    GetNArray(x_val, x_nary);

    /* Initialize some variables. */
    n_samples = (int)NA_SHAPE(x_nary)[0];
    n_features = (int)NA_SHAPE(x_nary)[1];
    y_shape[0] = n_samples;
    y_shape[1] = model->nr_class;
    y_val = rb_narray_new(numo_cDFloat, 2, y_shape);
    x_pt = (double*)na_get_pointer_for_read(x_val);
    y_pt = (double*)na_get_pointer_for_write(y_val);

    /* Predict values. */
    probs = ALLOC_N(double, model->nr_class);
    x_nodes = ALLOC_N(struct svm_node, n_features + 1);
    x_nodes[n_features].index = -1;
    x_nodes[n_features].value = 0.0;
    for (i = 0; i < n_samples; i++) {
      for (j = 0; j < n_features; j++) {
        x_nodes[j].index = j + 1;
        x_nodes[j].value = (double)x_pt[i * n_features + j];
      }
      svm_predict_probability(model, x_nodes, probs);
      for (j = 0; j < model->nr_class; j++) {
        y_pt[i * model->nr_class + j] = probs[j];
      }
    }
    xfree(x_nodes);
    xfree(probs);
  }

  xfree_svm_model(model);
  xfree_svm_parameter(param);

  return y_val;
}

/**
 * Load the SVM parameters and model from a text file with LIBSVM format.
 *
 * @param filename [String] The path to a file to load.
 * @return [Array] Array contains the SVM parameters and model.
 */
static
VALUE load_svm_model(VALUE self, VALUE filename)
{
  struct svm_model* model = svm_load_model(StringValuePtr(filename));
  VALUE res = rb_ary_new2(2);
  VALUE param_hash = Qnil;
  VALUE model_hash = Qnil;

  if (model) {
    param_hash = svm_parameter_to_rb_hash(&(model->param));
    model_hash = svm_model_to_rb_hash(model);
    svm_free_and_destroy_model(&model);
  }

  rb_ary_store(res, 0, param_hash);
  rb_ary_store(res, 1, model_hash);

  return res;
}

/**
 * Save the SVM parameters and model as a text file with LIBSVM format. The saved file can be used with the libsvm tools.
 *
 * @overload save_svm_model(filename, param, model) -> Boolean
 *
 * @param filename [String] The path to a file to save.
 * @param param [Hash] The parameters of the trained SVM model.
 * @param model [Hash] The model obtained from the training procedure.
 * @return [Boolean] true on success, or false if an error occurs.
 */
static
VALUE save_svm_model(VALUE self, VALUE filename, VALUE param_hash, VALUE model_hash)
{
  struct svm_parameter* param = rb_hash_to_svm_parameter(param_hash);
  struct svm_model* model = rb_hash_to_svm_model(model_hash);
  int res;

  model->param = *param;
  res = svm_save_model(StringValuePtr(filename), model);

  xfree_svm_model(model);
  xfree_svm_parameter(param);

  return res < 0 ? Qfalse : Qtrue;
}

void Init_libsvm()
{
  rb_require("numo/narray");

  mNumo = rb_define_module("Numo");
  mLibsvm = rb_define_module_under(mNumo, "Libsvm");

  /* The version of LIBSVM used in backgroud library. */
  rb_define_const(mLibsvm, "LIBSVM_VERSION", INT2NUM(LIBSVM_VERSION));

  /**
   * Document-module: Numo::Libsvm::SVM
   * SVM is a module that provides learning and prediction functions of LIBSVM.
   */
  mSvm = rb_define_module_under(mLibsvm, "SVM");

  rb_define_module_function(mSvm, "train", train, 3);
  rb_define_module_function(mSvm, "cv", cross_validation, 4);
  rb_define_module_function(mSvm, "predict", predict, 3);
  rb_define_module_function(mSvm, "decision_function", decision_function, 3);
  rb_define_module_function(mSvm, "predict_proba", predict_proba, 3);
  rb_define_module_function(mSvm, "load_svm_model", load_svm_model, 1);
  rb_define_module_function(mSvm, "save_svm_model", save_svm_model, 3);

  rb_init_svm_type_module();
  rb_init_kernel_type_module();
}
