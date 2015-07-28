%% Support Vector Regression class that implements the blackbox interface
classdef BlackBoxSVR < BlackBox
    properties
        model       = []
    end
    methods
        function blackBox = BlackBoxSVR(valley_cut)
            blackBox.type = 'r';
            blackBox.name = 'SVR';
            blackBox.valley_cut = valley_cut;
            blackBox.displayname = sprintf('SVR-valley-%.2f', valley_cut);
        end
        %% Returns a trained instance of a SVM model.
        %  parameters:
        %       parameter(1) = c = cost parameter for SVM learning
        %       parameter(2) = g = radius for the RBF kernel
        function blackBox = train    (blackBox,train_matrix, full_matrix, is_nominal)
            if isempty(blackBox.parameters)
                warning('No parameters specified, looking for optimal parameters now ...');
                blackBox.parameters = BlackBoxSVM.findParameters(train_matrix);
            end
            options             = sprintf('-s 4 -t 2 -m 400  -c %f -n %f',blackBox.parameters(1),blackBox.parameters(2));
            [~,blackBox.model]  = evalc('svmtrain(train_matrix(:,end),train_matrix(:,1:end-1),options)');
            
        end
        function [predictions,scores] = predict  (blackBox,test_instances)
            labels                  = zeros(size(test_instances,1),1);
            [~,predictions,~,scores]= evalc('svmpredict(labels,test_instances,blackBox.model)');
        end
    end
    
    methods(Static)
        function parameters = findParameters(train_set)
            train_instance  = train_set(:,1:end-1);
            train_label     = train_set(:,end);
            cutoff      = size(train_instance,1);
            C           = [-5:2:15];            
            G           = [-15:2:3];        
            steps       = 20;

            opt_c       = 0;
            best_mse    = Inf; %very nice coding here
            opt_gamma   = 0;    
            n_bad_its   = 0;
            for iter = 1 : 100
                fprintf('= Iteration %.0f =\n',iter);
                opt_found = 0;
                for c = C
                    for g = G
                        c2 = 2^c;
                        g2 = 2^g;

                        options     = sprintf('-q -s 4 -t 2 -m 400 -v 3 -c %f -n %f',c2,g2);
                        [~,mse]     = evalc('svmtrain(train_label,train_instance,options)');
                        if(mse < best_mse)
                            best_mse    = mse;
                            opt_c       = c;
                            opt_gamma   = g;
                            opt_found   = 1;
                            fprintf('c=%.5f \t g=%.5f mse=%.2f****\n',c2,g2,mse);
                        end
                    end
                end
                range = (max(C) - min(C))/iter;
                C     = opt_c     - range/2 : range/steps : opt_c + range/2;
                G     = opt_gamma - range/2 : range/steps : opt_gamma + range/2;
                if(~opt_found)
                    n_bad_its = n_bad_its+1;
                    if(n_bad_its >= 3)
                        break;
                    end        
                end
            end
            parameters = [2^opt_c , 2^opt_gamma];            
        end
    end
end