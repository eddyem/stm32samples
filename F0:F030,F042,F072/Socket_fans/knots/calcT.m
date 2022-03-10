function T = calcT(ADU)
%
% T = calcT(ADU) - piecewise calculation prototype
%
        X0 = [732.44    944.75   1197.23   2034.68   2308.60   2541.55   2738.63   2859.45   2969.21 3067.67   3153.52   3228.22   3292.84   3347.15];
        Y0 = [10.000   16.000   22.700   43.750   50.800   57.100   62.800   66.550   70.200   73.750   77.150   80.450   83.700   86.900];
        K  = [0.028261   0.026536   0.025136   0.025738   0.027044   0.028922   0.031038   0.033255 0.036056   0.039606   0.044173   0.050296   0.058920   0.071729];
        imax = max(size(X0)); idx = int32((imax+1)/2);
        T = [];
        % find index
        while(idx > 0 && idx <= imax)
                x = X0(idx);
                half = int32(idx/2);
                if(ADU < x)
                        %printf("%g < %g\n", ADU, x);
                        if(idx == 1) break; endif; % less than first index
                        if(ADU > X0(idx-1)) idx -= 1; break; endif; % found
                        idx = half; % another half
                else
                        %printf("%g > %g\n", ADU, x);
                        if(idx == imax) break; endif; % more than last index
                        if(ADU < X0(idx+1)) break; endif; % found
                        idx += half;
                endif
        endwhile
        if(idx < 1) printf("too low (<%g)!\n", X0(1)); idx = 1; endif
        if(idx > imax) idx = imax; endif;
        T = Y0(idx) + K(idx) * (ADU - X0(idx));
endfunction
