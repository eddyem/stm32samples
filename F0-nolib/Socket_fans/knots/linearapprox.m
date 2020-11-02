function [x0 y0 k] = linearapprox(X,Y)
% 
% [a b] = linearapprox(X,Y) - find linear approximation of function Y(X) through first and last points
%       y = y0 + k*(X-x0)
%
        x0 = X(1); y0 = Y(1);
        k = (Y(end) - y0) / (X(end) - x0);
endfunction
