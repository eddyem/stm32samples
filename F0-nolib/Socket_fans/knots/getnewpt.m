function idx = getnewpt(X, Y, delt)
%
% idx = getnewpt(X, Y, delt)
% find point where Y-`linear approx` is maximal
%       return index of max deviation or -1 if it is less than `delt`
%
        % make approximation
        [x0 y0 k] = linearapprox(X,Y);
        % find new knot
        adiff = abs(Y - (y0 + k*(X-x0)));
        maxadiff = max(adiff);
        if(maxadiff < delt)
                idx = -1;
        else
                idx = find(adiff == max(adiff));
        endif
endfunction
