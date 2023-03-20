function Tout = pt1000(Rin)
% Converts resistance of pt1000 into T (degrC)

T = [-200:0.1:50];
_A = 3.9083e-03;
_B = -5.7750e-07;
_C = zeros(size(T));
_C(find(T<0.)) = -4.1830e-12;
rT = 1000.*(1 + _A*T + _B*T.^2 - _C.*T.^3*100. + _C.*T.^4);
Tout = interp1(rT, T, Rin, 'spline');
endfunction
