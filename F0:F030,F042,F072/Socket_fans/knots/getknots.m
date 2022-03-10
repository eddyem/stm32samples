function idx = getknots(X, Y, dYmax)
%
% idx = getknots(X, Y, dYmax) - calculate piecewise-linear approximation knots for Y(X)
%       dYmax - maximal deviation 
%
        idx = [];
        I = getnewpt(X, Y, dYmax);
        if(I > 0)
                L = getknots(X(1:I), Y(1:I), dYmax);
                R = I-1 + getknots(X(I:end), Y(I:end), dYmax);
                idx = [L I R];
        endif
endfunction
