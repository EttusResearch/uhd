// All code take from the HDLCon paper:
// "Verilog Transcendental Functions for Numerical Testbenches"
//
// Authored by:
// Mark G. Arnold marnold@co.umist.ac.uk,
// Colin Walter c.walter@co.umist.ac.uk
// Freddy Engineer freddy.engineer@xilinx.com
//



// The sine function is approximated with a polynomial which works
// for -π/2 < x < π/2. (This polynomial, by itself, was used as a
// Verilog example in [2]; unfortunately there was a typo with the
// coefficients. The correct coefficients together with an error
// analysis are given in [3].)   For arguments outside of -π/2 < x < π/2,
// the identities sin(x) = -sin(-x) and sin(x) = -sin(x-π) allow the
// argument to be shifted to be within this range.  The latter identity
// can be applied repeatedly.  Doing so could cause inaccuracies for
// very large arguments, but in practice the errors are acceptable
// if the Verilog simulator uses double-precision floating point.

function real sin;
  input x;
  real x;
  real x1,y,y2,y3,y5,y7,sum,sign;
  begin
    sign = 1.0;
    x1 = x;
    if (x1<0)
      begin
        x1 = -x1;
        sign = -1.0;
      end
    while (x1 > 3.14159265/2.0)
      begin
        x1 = x1 - 3.14159265;
        sign = -1.0*sign;
      end
    y = x1*2/3.14159265;
    y2 = y*y;
    y3 = y*y2;
    y5 = y3*y2;
    y7 = y5*y2;
    sum = 1.570794*y - 0.645962*y3 +
           0.079692*y5 - 0.004681712*y7;
    sin = sign*sum;
  end
endfunction

// The cosine and tangent are computed from the sine:
function real cos;
  input x;
  real x;
  begin
    cos = sin(x + 3.14159265/2.0);
  end
endfunction


function real tan;
  input x;
  real x;
  begin
    tan = sin(x)/cos(x);
  end
endfunction

// The base-two exponential (antilogarithm) function, 2x, is computed by
// examining the bits of the argument, and for those bits of the argument
// that are 1, multiplying the result by the corresponding power of a base
//  very close to one.  For example,  if there were only two bits after
// the radix point, the base would be the fourth root of two, 1.1892.
// This number is squared on each iteration:  1.4142,  2.0,  4.0,  16.0.
// So, if x is 101.112, the function computes 25.75 as 1.1892*1.4142*2.0*16.0 = 53.81.
// In general, for k bits of precision, the base would be the 2k root of two.
// Since we need about 23 bits of accuracy for our function, the base we use
// is the 223 root of two, 1.000000082629586.  This constant poses a problem
// to some Verilog parsers, so we construct it in two parts.  The following
// function computes the appropriate root of two by repeatedly squaring this constant:

function real rootof2;
  input n;
  integer n;
  real power;
  integer i;

  begin
    power = 0.82629586;
    power = power / 10000000.0;
    power = power + 1.0;
    i = -23;

    if (n >= 1)
      begin
        power = 2.0;
        i = 0;
      end

    for (i=i; i< n; i=i+1)
      begin
        power = power * power;
      end
    rootof2 = power;
  end
endfunction // if

// This function is used for computing both antilogarithms and logarithms.
// This routine is never called with n less than -23, thus no validity check
// need be performed. When n>0, the exponentiation begins with 2.0 in order to
// improve accuracy.
// For computing the antilogarithm, we make use of the identity ex = 2x/ln(2),
// and then proceed as in the example above.  The constant 1/ln(2) = 1.44269504.
// Here is the natural exponential function:

function real exp;
  input x;
  real x;
  real x1,power,prod;
  integer i;
  begin
    x1 = fabs(x)*1.44269504;
    if (x1 > 255.0)
      begin
        exp = 0.0;
        if (x>0.0)
          begin
            $display("exp illegal argument:",x);
            $stop;
          end
      end
    else
      begin
        prod = 1.0;
        power = 128.0;
        for (i=7; i>=-23; i=i-1)
          begin
            if (x1 > power)
              begin
                prod = prod * rootof2(i);
                x1 = x1 - power;
              end
            power = power / 2.0;
          end
        if (x < 0)
          exp = 1.0/prod;
        else
          exp = prod;
      end
  end
endfunction // fabs

// The function prints an error message if the argument is too large
// (greater than about 180).  All error messages in this package are
// followed by $stop  to allow the designer to use the debugging
// features of Verilog to determine the cause of the error, and
// possibly to resume the simulation.  An argument of less than
// about –180 simply returns zero with no error.  The main loop
// assumes a positive argument.  A negative argument is computed as 1/e-x.
// The logarithm function prints an error message for arguments less
// than or equal to zero because the real-valued logarithm is not
// defined for such arguments.  The loop here requires an argument
// greater than or equal to one.  For arguments between zero and one,
// this code uses the identity ln(1/x) = -ln(x).

function real log;
  input x;
  real x;
  real re,log2;
  integer i;
  begin
    if (x <= 0.0)
      begin
        $display("log illegal argument:",x);
        $stop;
        log = 0;
      end
    else
      begin
        if (x<1.0)
          re = 1.0/x;
        else
          re = x;
        log2 = 0.0;
        for (i=7; i>=-23; i=i-1)
          begin
            if (re > rootof2(i))
              begin
                re = re/rootof2(i);
                log2 = 2.0*log2 + 1.0;
              end
            else
              log2 = log2*2;
          end
        if (x < 1.0)
          log = -log2/12102203.16;
        else
          log = log2/12102203.16;
      end
    end
endfunction

// The code only divides re by rootof2(i) when the re is larger
// (so that the quotient will be greater than 1.0). Each time
// such a division occurs, a bit that is 1 is recorded in the
// whole number result (multiply by 2 and add 1).  Otherwise,
// a zero is recorded (multiply by 2).  At the end of the loop,
// log2 will contain 223 log2|x|.  We divide by 223 and use the
// identity ln(x) = log2(x)/log2(e).  The constant 12102203.16 is 223  log2(e).
// The log(x) and exp(x)functions are used to implement the pow(x,y) and sqrt(x) functions:

function real pow;
  input x,y;
  real x,y;
  begin
    if (x<0.0)
      begin
        $display("pow illegal argument:",x);
        $stop;
      end
    pow = exp(y*log(x));
  end
endfunction

function real sqrt;
  input x;
  real x;
  begin
    if (x<0.0)
      begin
        $display("sqrt illegal argument:",x);
        $stop;
      end
    sqrt = exp(0.5*log(x));
  end
endfunction

// The arctangent [3,7] is computed as a continued fraction,
// using the identities tan-1(x) = -tan-1(-x) and tan-1(x) = π/2 - tan-1(1/x)
// to reduce the range to 0 < x < 1:

function real atan;
  input x;
  real x;
  real x1,x2,sign,bias;
  real d3,s3;
  begin
    sign = 1.0;
    bias = 0.0;
    x1 = x;
    if (x1 < 0.0)
      begin
        x1 = -x1;
        sign = -1.0;
      end
    if (x1 > 1.0)
      begin
        x1 = 1.0/x1;
        bias = sign*3.14159265/2.0;
        sign = -1.0*sign;
      end
    x2 = x1*x1;
    d3 = x2 + 1.44863154;
    d3 = 0.26476862 / d3;
    s3 = x2 + 3.3163354;
    d3 = s3 - d3;
    d3 = 7.10676 / d3;
    s3 = 6.762139 + x2;
    d3 = s3 - d3;
    d3 = 3.7092563 / d3;
    d3 = d3 + 0.17465544;
    atan = sign*x1*d3+bias;
  end
endfunction

// The other functions (asin(x) and acos(x)) are computed from the arctangent.
