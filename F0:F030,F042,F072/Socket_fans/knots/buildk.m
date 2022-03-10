function [X0 Y0 K] = buildk(X, Y, dYmax)
%
% [X0 Y0 K] = buildk(X, Y, dYmax) - build arrays of knots (X0, Y0) and linear koefficients K
%       for piecewise-linear approximation of Y(X) with max deviation `dYmax`
%
        X0 = []; Y0 = []; K = [];
        knots = [1 getknots(X, Y, dYmax) max(size(X))];
        for i = 1:max(size(knots))-1
                idx = knots(i):knots(i+1);
                [x y k] = linearapprox(X(idx), Y(idx));
                X0 = [X0 x]; Y0 = [Y0 y]; K = [K k];
        endfor
endfunction
