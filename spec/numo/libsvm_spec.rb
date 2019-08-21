# frozen_string_literal: true

RSpec.describe Numo::Libsvm do
  describe 'constant values' do
    it 'has version numbers', aggregate_failures: true do
      expect(Numo::Libsvm::VERSION).not_to be nil
      expect(Numo::Libsvm::LIBSVM_VERSION).not_to be nil
    end

    it 'has svm and kernel type values', aggregate_failures: true do
      expect(Numo::Libsvm::SvmType::C_SVC).to eq(0)
      expect(Numo::Libsvm::SvmType::NU_SVC).to eq(1)
      expect(Numo::Libsvm::SvmType::ONE_CLASS).to eq(2)
      expect(Numo::Libsvm::SvmType::EPSILON_SVR).to eq(3)
      expect(Numo::Libsvm::SvmType::NU_SVR).to eq(4)
      expect(Numo::Libsvm::KernelType::LINEAR).to eq(0)
      expect(Numo::Libsvm::KernelType::POLY).to eq(1)
      expect(Numo::Libsvm::KernelType::RBF).to eq(2)
      expect(Numo::Libsvm::KernelType::SIGMOID).to eq(3)
      expect(Numo::Libsvm::KernelType::PRECOMPUTED).to eq(4)
    end
  end

  describe 'classification' do
    let(:dataset) { Marshal.load(File.read(__dir__ + '/../iris.dat')) }
    let(:x) { dataset[0] }
    let(:y) { dataset[1] }
    let(:x_test) { dataset[2] }
    let(:y_test) { dataset[3] }
    let(:classes) { Numo::Int32[*y.to_a.uniq] }
    let(:n_classes) { classes.size }
    let(:n_test_samples) { x_test.shape[0] }
    let(:c_svc_model) { Numo::Libsvm.train(x, y, c_svc_param) }
    let(:c_svc_param) do
      { svm_type: Numo::Libsvm::SvmType::C_SVC,
        kernel_type: Numo::Libsvm::KernelType::SIGMOID,
        gamma: 0.1,
        coef0: 1,
        C: 10,
        shrinking: true,
        probability: true }
    end

    it 'performs 5-cross validation with C-SVC' do
      pr = Numo::Libsvm.cv(x, y, c_svc_param, 5)
      expect(accuracy(y, pr)).to be_within(0.05).of(0.95)
    end

    it 'calculates decision function with C-SVC' do
      df = Numo::Libsvm.decision_function(x_test, c_svc_param, c_svc_model)
      expect(df.class).to eq(Numo::DFloat)
      expect(df.shape[0]).to eq(n_test_samples)
      expect(df.shape[1]).to eq(n_classes * (n_classes - 1) / 2)
    end

    it 'predicts probabilities with C-SVC' do
      pb = Numo::Libsvm.predict_proba(x_test, c_svc_param, c_svc_model)
      pr = Numo::Int32[*(Array.new(n_test_samples) { |n| classes[pb[n, true].max_index] })]
      expect(pb.class).to eq(Numo::DFloat)
      expect(pb.shape[0]).to eq(n_test_samples)
      expect(pb.shape[1]).to eq(n_classes)
      expect(accuracy(y_test, pr)).to be_within(0.05).of(0.95)
    end

    it 'predicts labels with C-SVC' do
      pr = Numo::Libsvm.predict(x_test, c_svc_param, c_svc_model)
      expect(pr.class).to eq(Numo::DFloat)
      expect(pr.shape[0]).to eq(n_test_samples)
      expect(pr.shape[1]).to be_nil
      expect(accuracy(y_test, pr)).to be_within(0.05).of(0.95)
    end
  end

  describe 'regression' do
    let(:dataset) { Marshal.load(File.read(__dir__ + '/../housing.dat')) }
    let(:x) { dataset[0] }
    let(:y) { dataset[1] }
    let(:x_test) { dataset[2] }
    let(:y_test) { dataset[3] }
    let(:n_test_samples) { x_test.shape[0] }
    let(:svr_model) { Numo::Libsvm.train(x, y, svr_param) }
    let(:svr_param) do
      { svm_type: Numo::Libsvm::SvmType::EPSILON_SVR,
        kernel_type: Numo::Libsvm::KernelType::RBF,
        gamma: 0.0001,
        C: 10,
        p: 0.0001,
        shrinking: true }
    end

    it 'predicts target values with SVR', aggregate_failures: true do
      pr = Numo::Libsvm.predict(x_test, svr_param, svr_model)
      expect(pr.class).to eq(Numo::DFloat)
      expect(pr.shape[0]).to eq(n_test_samples)
      expect(pr.shape[1]).to be_nil
      expect(r2_score(y_test, pr)).to be >= 0.5
    end

    it 'calculates decision function with SVR', aggregate_failures: true do
      df = Numo::Libsvm.decision_function(x_test, svr_param, svr_model)
      pr = Numo::Libsvm.predict(x_test, svr_param, svr_model)
      err = (df - pr).abs.mean
      expect(df.class).to eq(Numo::DFloat)
      expect(df.shape[0]).to eq(n_test_samples)
      expect(df.shape[1]).to be_nil
      expect(err).to be <= 1e-8
    end

    it 'performs 5-cross validation with SVR' do
      pr = Numo::Libsvm.cv(x, y, svr_param, 5)
      expect(r2_score(y, pr)).to be >= 0.1
    end
  end

  describe 'distribution estimation' do
    let(:dataset) { Marshal.load(File.read(__dir__ + '/../diabetes.dat')) }
    let(:pos_id) { dataset[1].eq(1).where }
    let(:neg_id) { dataset[1].ne(1).where }
    let(:x_pos) { dataset[0][pos_id, true] }
    let(:y_pos) { dataset[1][pos_id] }
    let(:x_neg) { dataset[0][neg_id, true] }
    let(:y_neg) { dataset[1][neg_id] }
    let(:n_test_samples) { x_neg.shape[0] }
    let(:oc_svm_model) { Numo::Libsvm.train(x_pos, y_pos, oc_svm_param) }
    let(:oc_svm_param) do
      { svm_type: Numo::Libsvm::SvmType::ONE_CLASS,
        kernel_type: Numo::Libsvm::KernelType::RBF,
        gamma: 1.0,
        nu: 0.5 }
    end

    it 'calculates decision function with one-class SVM' do
      df = Numo::Libsvm.decision_function(x_neg, oc_svm_param, oc_svm_model)
      expect(df.class).to eq(Numo::DFloat)
      expect(df.shape[0]).to eq(n_test_samples)
      expect(df.shape[1]).to be_nil
      expect(df.lt(0.0).count).to eq(n_test_samples)
    end

    it 'predicts labels with one-class SVM' do
      pr = Numo::Libsvm.predict(x_neg, oc_svm_param, oc_svm_model)
      expect(pr.class).to eq(Numo::DFloat)
      expect(pr.shape[0]).to eq(n_test_samples)
      expect(pr.shape[1]).to be_nil
      expect(accuracy(y_neg, pr)).to be >= 0.9
    end
  end

  describe 'errors' do
    let(:dataset) { Marshal.load(File.read(__dir__ + '/../iris.dat')) }
    let(:x) { dataset[0] }
    let(:y) { dataset[1] }
    let(:svm_model) { Numo::Libsvm.train(x, y, svm_param) }
    let(:svm_param) do
      { svm_type: Numo::Libsvm::SvmType::C_SVC,
        kernel_type: Numo::Libsvm::KernelType::SIGMOID,
        gamma: 0.1,
        coef0: 1,
        C: 10,
        shrinking: true,
        probability: true }
    end

    describe '#train' do
      it 'raises ArgumentError when given non two-dimensional array as sample array' do
        expect { described_class.train(Numo::DFloat.new(3, 2, 2).rand, Numo::DFloat.new(3).rand, svm_param) }.to raise_error(ArgumentError, 'Expect samples to be 2-D array.')
      end

      it 'raises ArgumentError when given non one-dimensional array as label array' do
        expect { described_class.train(Numo::DFloat.new(3, 2).rand, Numo::DFloat.new(3, 2).rand, svm_param) }.to raise_error(ArgumentError, 'Expect label or target values to be 1-D arrray.')
      end

      it 'raises ArgumentError when the number of smaples of sample array and label array are different' do
        expect { described_class.train(Numo::DFloat.new(5, 2).rand, Numo::DFloat.new(3).rand, svm_param) }.to raise_error(ArgumentError, 'Expect to have the same number of samples for samples and labels.')
        expect { described_class.train(Numo::DFloat.new(3, 2).rand, Numo::DFloat.new(5).rand, svm_param) }.to raise_error(ArgumentError, 'Expect to have the same number of samples for samples and labels.')
      end

      it 'raises ArgumentError when given invalid parameter value for libsvm' do
        svm_param[:gamma] = -100
        expect { described_class.train(Numo::DFloat.new(3, 2).rand, Numo::DFloat.new(3).rand, svm_param) }.to raise_error(ArgumentError, 'Invalid LIBSVM parameter is given: gamma < 0')
      end
    end

    describe '#cv' do
      it 'raises ArgumentError when given non two-dimensional array as sample array' do
        expect { described_class.cv(Numo::DFloat.new(3, 2, 2).rand, Numo::DFloat.new(3).rand, svm_param, 5) }.to raise_error(ArgumentError, 'Expect samples to be 2-D array.')
      end

      it 'raises ArgumentError when given non one-dimensional array as label array' do
        expect { described_class.cv(Numo::DFloat.new(3, 2).rand, Numo::DFloat.new(3, 2).rand, svm_param, 5) }.to raise_error(ArgumentError, 'Expect label or target values to be 1-D arrray.')
      end

      it 'raises ArgumentError when the number of smaples of sample array and label array are different' do
        expect { described_class.cv(Numo::DFloat.new(5, 2).rand, Numo::DFloat.new(3).rand, svm_param, 5) }.to raise_error(ArgumentError, 'Expect to have the same number of samples for samples and labels.')
        expect { described_class.cv(Numo::DFloat.new(3, 2).rand, Numo::DFloat.new(5).rand, svm_param, 5) }.to raise_error(ArgumentError, 'Expect to have the same number of samples for samples and labels.')
      end

      it 'raises ArgumentError when given invalid parameter value for libsvm' do
        svm_param[:gamma] = -100
        expect { described_class.cv(Numo::DFloat.new(3, 2).rand, Numo::DFloat.new(3).rand, svm_param, 5) }.to raise_error(ArgumentError, 'Invalid LIBSVM parameter is given: gamma < 0')
      end
    end

    describe '#predict' do
      it 'raises ArgumentError when given non two-dimensional array as sample array' do
        expect { described_class.predict(Numo::DFloat.new(3, 2, 2).rand, svm_param, svm_model) }.to raise_error(ArgumentError, 'Expect samples to be 2-D array.')
      end
    end

    describe '#decision_function' do
      it 'raises ArgumentError when given non two-dimensional array as sample array' do
        expect { described_class.decision_function(Numo::DFloat.new(3, 2, 2).rand, svm_param, svm_model) }.to raise_error(ArgumentError, 'Expect samples to be 2-D array.')
      end
    end

    describe '#predict_proba' do
      it 'raises ArgumentError when given non two-dimensional array as sample array' do
        expect { described_class.predict_proba(Numo::DFloat.new(3, 2, 2).rand, svm_param, svm_model) }.to raise_error(ArgumentError, 'Expect samples to be 2-D array.')
      end
    end

    describe '#load_svm_model' do
      it 'raises IOError when failed load file' do
        expect { described_class.load_svm_model('foo') }.to raise_error(IOError, "Failed to load file 'foo'")
      end
    end

    describe '#save_svm_model' do
      it 'raises IOError when failed save file' do
        expect { described_class.save_svm_model('', svm_param, svm_model) }.to raise_error(IOError, "Failed to save file ''")
      end
    end
  end
end
