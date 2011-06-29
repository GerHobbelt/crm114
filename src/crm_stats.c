//  crm_stats.c
//
//  This software is licensed to the public under the Free Software
//  Foundation's GNU GPL, version 2.  You may obtain a copy of the
//  GPL by visiting the Free Software Foundations web site at
//  www.fsf.org, and a copy is included in this distribution.
//
//     This code is dual-licensed to both William S. Yerazunis and Joe
//     Langeway, including the right to reuse this code in any way
//     desired, including the right to relicense it under any other
//     terms as desired.
//
//  Other licenses may be negotiated; contact the
//  author for details.
//
//  include some standard files
#include "crm114_sysincludes.h"
//  include any local crm114 configuration file
#include "crm114_config.h"
//  include the crm114 data structures file
#include "crm114_structs.h"
//  and include the routine declarations file
#include "crm114.h"


static const double norm_cdf_lookup[] =
{
    9.865876e-10, 1.086112e-09, 1.195391e-09, 1.315351e-09, 1.447005e-09, 1.591458e-09, 1.749914e-09, 1.923689e-09, 2.114217e-09, 2.323062e-09,
    2.551930e-09, 2.802679e-09, 3.077334e-09, 3.378100e-09, 3.707380e-09, 4.067789e-09, 4.462172e-09, 4.893629e-09, 5.365527e-09, 5.881533e-09,
    6.445630e-09, 7.062151e-09, 7.735803e-09, 8.471701e-09, 9.275399e-09, 1.015293e-08, 1.111084e-08, 1.215625e-08, 1.329685e-08, 1.454102e-08,
    1.589784e-08, 1.737713e-08, 1.898956e-08, 2.074669e-08, 2.266102e-08, 2.474613e-08, 2.701668e-08, 2.948856e-08, 3.217898e-08, 3.510653e-08,
    3.829134e-08, 4.175518e-08, 4.552156e-08, 4.961591e-08, 5.406571e-08, 5.890064e-08, 6.415274e-08, 6.985661e-08, 7.604961e-08, 8.277203e-08,
    9.006736e-08, 9.798248e-08, 1.065680e-07, 1.158783e-07, 1.259722e-07, 1.369130e-07, 1.487689e-07, 1.616131e-07, 1.755248e-07, 1.905889e-07,
    2.068970e-07, 2.245475e-07, 2.436461e-07, 2.643067e-07, 2.866516e-07, 3.108121e-07, 3.369294e-07, 3.651551e-07, 3.956520e-07, 4.285948e-07,
    4.641709e-07, 5.025815e-07, 5.440423e-07, 5.887845e-07, 6.370561e-07, 6.891229e-07, 7.452694e-07, 8.058005e-07, 8.710428e-07, 9.413457e-07,
    1.017083e-06, 1.098656e-06, 1.186491e-06, 1.281047e-06, 1.382814e-06, 1.492313e-06, 1.610104e-06, 1.736785e-06, 1.872992e-06, 2.019406e-06,
    2.176754e-06, 2.345812e-06, 2.527405e-06, 2.722416e-06, 2.931785e-06, 3.156515e-06, 3.397673e-06, 3.656398e-06, 3.933901e-06, 4.231473e-06,
    4.550486e-06, 4.892403e-06, 5.258778e-06, 5.651266e-06, 6.071624e-06, 6.521722e-06, 7.003545e-06, 7.519206e-06, 8.070944e-06, 8.661140e-06,
    9.292321e-06, 9.967168e-06, 1.068853e-05, 1.145941e-05, 1.228302e-05, 1.316276e-05, 1.410220e-05, 1.510517e-05, 1.617569e-05, 1.731804e-05,
    1.853674e-05, 1.983657e-05, 2.122260e-05, 2.270018e-05, 2.427497e-05, 2.595297e-05, 2.774050e-05, 2.964423e-05, 3.167124e-05, 3.382898e-05,
    3.612532e-05, 3.856856e-05, 4.116747e-05, 4.393129e-05, 4.686977e-05, 4.999318e-05, 5.331235e-05, 5.683869e-05, 6.058422e-05, 6.456159e-05,
    6.878411e-05, 7.326582e-05, 7.802144e-05, 8.306649e-05, 8.841729e-05, 9.409096e-05, 1.001055e-04, 1.064799e-04, 1.132340e-04, 1.203887e-04,
    1.279659e-04, 1.359885e-04, 1.444807e-04, 1.534678e-04, 1.629763e-04, 1.730340e-04, 1.836700e-04, 1.949148e-04, 2.068003e-04, 2.193601e-04,
    2.326291e-04, 2.466439e-04, 2.614429e-04, 2.770661e-04, 2.935554e-04, 3.109545e-04, 3.293092e-04, 3.486672e-04, 3.690785e-04, 3.905949e-04,
    4.132709e-04, 4.371631e-04, 4.623306e-04, 4.888350e-04, 5.167405e-04, 5.461139e-04, 5.770250e-04, 6.095464e-04, 6.437534e-04, 6.797248e-04,
    7.175423e-04, 7.572909e-04, 7.990591e-04, 8.429387e-04, 8.890253e-04, 9.374180e-04, 9.882198e-04, 1.041538e-03, 1.097482e-03, 1.156169e-03,
    1.217718e-03, 1.282251e-03, 1.349898e-03, 1.420791e-03, 1.495069e-03, 1.572873e-03, 1.654351e-03, 1.739656e-03, 1.828945e-03, 1.922383e-03,
    2.020137e-03, 2.122383e-03, 2.229301e-03, 2.341076e-03, 2.457901e-03, 2.579975e-03, 2.707501e-03, 2.840691e-03, 2.979763e-03, 3.124941e-03,
    3.276456e-03, 3.434545e-03, 3.599455e-03, 3.771437e-03, 3.950751e-03, 4.137663e-03, 4.332448e-03, 4.535389e-03, 4.746775e-03, 4.966903e-03,
    5.196079e-03, 5.434618e-03, 5.682840e-03, 5.941077e-03, 6.209665e-03, 6.488953e-03, 6.779295e-03, 7.081056e-03, 7.394607e-03, 7.720330e-03,
    8.058616e-03, 8.409861e-03, 8.774475e-03, 9.152873e-03, 9.545482e-03, 9.952734e-03, 1.037507e-02, 1.081295e-02, 1.126683e-02, 1.173718e-02,
    1.222447e-02, 1.272920e-02, 1.325187e-02, 1.379297e-02, 1.435302e-02, 1.493255e-02, 1.553207e-02, 1.615215e-02, 1.679331e-02, 1.745611e-02,
    1.814113e-02, 1.884892e-02, 1.958008e-02, 2.033518e-02, 2.111482e-02, 2.191960e-02, 2.275013e-02, 2.360703e-02, 2.449090e-02, 2.540239e-02,
    2.634213e-02, 2.731074e-02, 2.830889e-02, 2.933721e-02, 3.039636e-02, 3.148700e-02, 3.260979e-02, 3.376540e-02, 3.495449e-02, 3.617773e-02,
    3.743581e-02, 3.872939e-02, 4.005916e-02, 4.142578e-02, 4.282995e-02, 4.427234e-02, 4.575362e-02, 4.727449e-02, 4.883560e-02, 5.043764e-02,
    5.208128e-02, 5.376718e-02, 5.549602e-02, 5.726845e-02, 5.908512e-02, 6.094670e-02, 6.285381e-02, 6.480710e-02, 6.680720e-02, 6.885473e-02,
    7.095031e-02, 7.309453e-02, 7.528799e-02, 7.753127e-02, 7.982495e-02, 8.216959e-02, 8.456572e-02, 8.701389e-02, 8.951462e-02, 9.206841e-02,
    9.467574e-02, 9.733710e-02, 1.000529e-01, 1.028237e-01, 1.056498e-01, 1.085316e-01, 1.114695e-01, 1.144640e-01, 1.175152e-01, 1.206236e-01,
    1.237895e-01, 1.270130e-01, 1.302945e-01, 1.336342e-01, 1.370323e-01, 1.404890e-01, 1.440044e-01, 1.475786e-01, 1.512118e-01, 1.549040e-01,
    1.586553e-01, 1.624656e-01, 1.663350e-01, 1.702634e-01, 1.742507e-01, 1.782969e-01, 1.824018e-01, 1.865652e-01, 1.907870e-01, 1.950668e-01,
    1.994046e-01, 2.037999e-01, 2.082524e-01, 2.127618e-01, 2.173277e-01, 2.219497e-01, 2.266274e-01, 2.313601e-01, 2.361475e-01, 2.409889e-01,
    2.458839e-01, 2.508316e-01, 2.558316e-01, 2.608832e-01, 2.659855e-01, 2.711380e-01, 2.763397e-01, 2.815899e-01, 2.868877e-01, 2.922323e-01,
    2.976228e-01, 3.030582e-01, 3.085375e-01, 3.140599e-01, 3.196242e-01, 3.252294e-01, 3.308744e-01, 3.365581e-01, 3.422795e-01, 3.480372e-01,
    3.538302e-01, 3.596573e-01, 3.655172e-01, 3.714086e-01, 3.773303e-01, 3.832810e-01, 3.892593e-01, 3.952640e-01, 4.012937e-01, 4.073469e-01,
    4.134224e-01, 4.195187e-01, 4.256343e-01, 4.317679e-01, 4.379180e-01, 4.440831e-01, 4.502618e-01, 4.564525e-01, 4.626539e-01, 4.688643e-01,
    4.750823e-01, 4.813064e-01, 4.875351e-01, 4.937668e-01, 5.000000e-01, 5.062332e-01, 5.124649e-01, 5.186936e-01, 5.249177e-01, 5.311357e-01,
    5.373461e-01, 5.435475e-01, 5.497382e-01, 5.559169e-01, 5.620820e-01, 5.682321e-01, 5.743657e-01, 5.804813e-01, 5.865776e-01, 5.926531e-01,
    5.987063e-01, 6.047360e-01, 6.107407e-01, 6.167190e-01, 6.226697e-01, 6.285914e-01, 6.344828e-01, 6.403427e-01, 6.461698e-01, 6.519628e-01,
    6.577205e-01, 6.634419e-01, 6.691256e-01, 6.747706e-01, 6.803758e-01, 6.859401e-01, 6.914625e-01, 6.969418e-01, 7.023772e-01, 7.077677e-01,
    7.131123e-01, 7.184101e-01, 7.236603e-01, 7.288620e-01, 7.340145e-01, 7.391168e-01, 7.441684e-01, 7.491684e-01, 7.541161e-01, 7.590111e-01,
    7.638525e-01, 7.686399e-01, 7.733726e-01, 7.780503e-01, 7.826723e-01, 7.872382e-01, 7.917476e-01, 7.962001e-01, 8.005954e-01, 8.049332e-01,
    8.092130e-01, 8.134348e-01, 8.175982e-01, 8.217031e-01, 8.257493e-01, 8.297366e-01, 8.336650e-01, 8.375344e-01, 8.413447e-01, 8.450960e-01,
    8.487882e-01, 8.524214e-01, 8.559956e-01, 8.595110e-01, 8.629677e-01, 8.663658e-01, 8.697055e-01, 8.729870e-01, 8.762105e-01, 8.793764e-01,
    8.824848e-01, 8.855360e-01, 8.885305e-01, 8.914684e-01, 8.943502e-01, 8.971763e-01, 8.999471e-01, 9.026629e-01, 9.053243e-01, 9.079316e-01,
    9.104854e-01, 9.129861e-01, 9.154343e-01, 9.178304e-01, 9.201750e-01, 9.224687e-01, 9.247120e-01, 9.269055e-01, 9.290497e-01, 9.311453e-01,
    9.331928e-01, 9.351929e-01, 9.371462e-01, 9.390533e-01, 9.409149e-01, 9.427316e-01, 9.445040e-01, 9.462328e-01, 9.479187e-01, 9.495624e-01,
    9.511644e-01, 9.527255e-01, 9.542464e-01, 9.557277e-01, 9.571700e-01, 9.585742e-01, 9.599408e-01, 9.612706e-01, 9.625642e-01, 9.638223e-01,
    9.650455e-01, 9.662346e-01, 9.673902e-01, 9.685130e-01, 9.696036e-01, 9.706628e-01, 9.716911e-01, 9.726893e-01, 9.736579e-01, 9.745976e-01,
    9.755091e-01, 9.763930e-01, 9.772499e-01, 9.780804e-01, 9.788852e-01, 9.796648e-01, 9.804199e-01, 9.811511e-01, 9.818589e-01, 9.825439e-01,
    9.832067e-01, 9.838479e-01, 9.844679e-01, 9.850675e-01, 9.856470e-01, 9.862070e-01, 9.867481e-01, 9.872708e-01, 9.877755e-01, 9.882628e-01,
    9.887332e-01, 9.891870e-01, 9.896249e-01, 9.900473e-01, 9.904545e-01, 9.908471e-01, 9.912255e-01, 9.915901e-01, 9.919414e-01, 9.922797e-01,
    9.926054e-01, 9.929189e-01, 9.932207e-01, 9.935110e-01, 9.937903e-01, 9.940589e-01, 9.943172e-01, 9.945654e-01, 9.948039e-01, 9.950331e-01,
    9.952532e-01, 9.954646e-01, 9.956676e-01, 9.958623e-01, 9.960492e-01, 9.962286e-01, 9.964005e-01, 9.965655e-01, 9.967235e-01, 9.968751e-01,
    9.970202e-01, 9.971593e-01, 9.972925e-01, 9.974200e-01, 9.975421e-01, 9.976589e-01, 9.977707e-01, 9.978776e-01, 9.979799e-01, 9.980776e-01,
    9.981711e-01, 9.982603e-01, 9.983456e-01, 9.984271e-01, 9.985049e-01, 9.985792e-01, 9.986501e-01, 9.987177e-01, 9.987823e-01, 9.988438e-01,
    9.989025e-01, 9.989585e-01, 9.990118e-01, 9.990626e-01, 9.991110e-01, 9.991571e-01, 9.992009e-01, 9.992427e-01, 9.992825e-01, 9.993203e-01,
    9.993562e-01, 9.993905e-01, 9.994230e-01, 9.994539e-01, 9.994833e-01, 9.995112e-01, 9.995377e-01, 9.995628e-01, 9.995867e-01, 9.996094e-01,
    9.996309e-01, 9.996513e-01, 9.996707e-01, 9.996890e-01, 9.997064e-01, 9.997229e-01, 9.997386e-01, 9.997534e-01, 9.997674e-01, 9.997806e-01,
    9.997932e-01, 9.998051e-01, 9.998163e-01, 9.998270e-01, 9.998370e-01, 9.998465e-01, 9.998555e-01, 9.998640e-01, 9.998720e-01, 9.998796e-01,
    9.998868e-01, 9.998935e-01, 9.998999e-01, 9.999059e-01, 9.999116e-01, 9.999169e-01, 9.999220e-01, 9.999267e-01, 9.999312e-01, 9.999354e-01,
    9.999394e-01, 9.999432e-01, 9.999467e-01, 9.999500e-01, 9.999531e-01, 9.999561e-01, 9.999588e-01, 9.999614e-01, 9.999639e-01, 9.999662e-01,
    9.999683e-01, 9.999704e-01, 9.999723e-01, 9.999740e-01, 9.999757e-01, 9.999773e-01, 9.999788e-01, 9.999802e-01, 9.999815e-01, 9.999827e-01,
    9.999838e-01, 9.999849e-01, 9.999859e-01, 9.999868e-01, 9.999877e-01, 9.999885e-01, 9.999893e-01, 9.999900e-01, 9.999907e-01, 9.999913e-01,
    9.999919e-01, 9.999925e-01, 9.999930e-01, 9.999935e-01, 9.999939e-01, 9.999943e-01, 9.999947e-01, 9.999951e-01, 9.999954e-01, 9.999958e-01,
    9.999961e-01, 9.999963e-01, 9.999966e-01, 9.999968e-01, 9.999971e-01, 9.999973e-01, 9.999975e-01, 9.999977e-01, 9.999978e-01, 9.999980e-01,
    9.999981e-01, 9.999983e-01, 9.999984e-01, 9.999985e-01, 9.999986e-01, 9.999987e-01, 9.999988e-01, 9.999989e-01, 9.999990e-01, 9.999991e-01,
    9.999991e-01, 9.999992e-01, 9.999993e-01, 9.999993e-01, 9.999994e-01, 9.999994e-01, 9.999995e-01, 9.999995e-01, 9.999995e-01, 9.999996e-01,
    9.999996e-01, 9.999996e-01, 9.999997e-01, 9.999997e-01, 9.999997e-01, 9.999997e-01, 9.999998e-01, 9.999998e-01, 9.999998e-01, 9.999998e-01,
    9.999998e-01, 9.999998e-01, 9.999999e-01, 9.999999e-01, 9.999999e-01, 9.999999e-01, 9.999999e-01, 9.999999e-01, 9.999999e-01, 9.999999e-01,
    9.999999e-01, 9.999999e-01, 9.999999e-01, 9.999999e-01, 9.999999e-01, 1.000000e-00, 1.000000e-00, 1.000000e-00, 1.000000e-00, 1.000000e-00,
    1.000000e-00, 1.000000e-00, 1.000000e-00, 1.000000e-00, 1.000000e-00, 1.000000e-00, 1.000000e-00, 1.000000e-00, 1.000000e-00, 1.000000e-00,
    1.000000e-00, 1.000000e-00, 1.000000e-00, 1.000000e-00, 1.000000e-00, 1.000000e-00, 1.000000e-00, 1.000000e-00, 1.000000e-00, 1.000000e-00,
    1.000000e-00, 1.000000e-00, 1.000000e-00, 1.000000e-00, 1.000000e-00, 1.000000e-00, 1.000000e-00, 1.000000e-00, 1.000000e-00, 1.000000e-00,
    1.000000e-00, 1.000000e-00, 1.000000e-00, 1.000000e-00, 1.000000e-00, 1.000000e-00, 1.000000e-00, 1.000000e-00
};



//there is currently no interpolation
double crm_norm_cdf(double x)
{
    int i;

    if (x < -6.0)
        return 0.0;

    if (x >= 6.0)
        return 1.0;

    i = (int)((x + 6.0) * 32.0);
    return norm_cdf_lookup[i];
}

//  notice we put -7.0 in place of -inf, monotonicity is all that
//  matters for the algorithm, and huge extrema would only make
//  visualization harder


static const double log_lookup_table[] =
{
    -7.0, -5.950643e+00, -5.257495e+00, -4.852030e+00, -4.564348e+00, -4.341205e+00, -4.158883e+00, -4.004732e+00, -3.871201e+00, -3.753418e+00,
    -3.648057e+00, -3.552747e+00, -3.465736e+00, -3.385693e+00, -3.311585e+00, -3.242592e+00, -3.178054e+00, -3.117429e+00, -3.060271e+00,
    -3.006204e+00, -2.954910e+00, -2.906120e+00, -2.859600e+00, -2.815148e+00, -2.772589e+00, -2.731767e+00, -2.692546e+00, -2.654806e+00,
    -2.618438e+00, -2.583347e+00, -2.549445e+00, -2.516655e+00, -2.484907e+00, -2.454135e+00, -2.424282e+00, -2.395294e+00, -2.367124e+00,
    -2.339725e+00, -2.313056e+00, -2.287081e+00, -2.261763e+00, -2.237070e+00, -2.212973e+00, -2.189442e+00, -2.166453e+00, -2.143980e+00,
    -2.122001e+00, -2.100495e+00, -2.079442e+00, -2.058822e+00, -2.038620e+00, -2.018817e+00, -1.999399e+00, -1.980351e+00, -1.961659e+00,
    -1.943309e+00, -1.925291e+00, -1.907591e+00, -1.890200e+00, -1.873105e+00, -1.856298e+00, -1.839769e+00, -1.823508e+00, -1.807508e+00,
    -1.791759e+00, -1.776255e+00, -1.760988e+00, -1.745950e+00, -1.731135e+00, -1.716536e+00, -1.702147e+00, -1.687963e+00, -1.673976e+00,
    -1.660183e+00, -1.646577e+00, -1.633154e+00, -1.619909e+00, -1.606837e+00, -1.593934e+00, -1.581195e+00, -1.568616e+00, -1.556193e+00,
    -1.543923e+00, -1.531802e+00, -1.519826e+00, -1.507991e+00, -1.496295e+00, -1.484734e+00, -1.473306e+00, -1.462006e+00, -1.450833e+00,
    -1.439783e+00, -1.428854e+00, -1.418043e+00, -1.407348e+00, -1.396766e+00, -1.386294e+00, -1.375932e+00, -1.365675e+00, -1.355523e+00,
    -1.345472e+00, -1.335522e+00, -1.325670e+00, -1.315914e+00, -1.306252e+00, -1.296682e+00, -1.287203e+00, -1.277814e+00, -1.268511e+00,
    -1.259295e+00, -1.250162e+00, -1.241112e+00, -1.232144e+00, -1.223255e+00, -1.214444e+00, -1.205710e+00, -1.197052e+00, -1.188469e+00,
    -1.179958e+00, -1.171519e+00, -1.163151e+00, -1.154852e+00, -1.146622e+00, -1.138458e+00, -1.130361e+00, -1.122329e+00, -1.114361e+00,
    -1.106455e+00, -1.098612e+00, -1.090830e+00, -1.083108e+00, -1.075445e+00, -1.067841e+00, -1.060293e+00, -1.052803e+00, -1.045368e+00,
    -1.037988e+00, -1.030662e+00, -1.023389e+00, -1.016169e+00, -1.009000e+00, -1.001883e+00, -9.948155e-01, -9.877979e-01, -9.808293e-01,
    -9.739088e-01, -9.670359e-01, -9.602100e-01, -9.534303e-01, -9.466962e-01, -9.400073e-01, -9.333627e-01, -9.267620e-01, -9.202046e-01,
    -9.136900e-01, -9.072174e-01, -9.007865e-01, -8.943967e-01, -8.880475e-01, -8.817384e-01, -8.754687e-01, -8.692382e-01, -8.630462e-01,
    -8.568924e-01, -8.507761e-01, -8.446971e-01, -8.386548e-01, -8.326487e-01, -8.266786e-01, -8.207438e-01, -8.148441e-01, -8.089790e-01,
    -8.031481e-01, -7.973510e-01, -7.915873e-01, -7.858566e-01, -7.801586e-01, -7.744928e-01, -7.688590e-01, -7.632567e-01, -7.576857e-01,
    -7.521455e-01, -7.466359e-01, -7.411564e-01, -7.357068e-01, -7.302867e-01, -7.248959e-01, -7.195339e-01, -7.142006e-01, -7.088955e-01,
    -7.036185e-01, -6.983691e-01, -6.931472e-01, -6.879524e-01, -6.827844e-01, -6.776430e-01, -6.725279e-01, -6.674388e-01, -6.623755e-01,
    -6.573377e-01, -6.523252e-01, -6.473376e-01, -6.423749e-01, -6.374366e-01, -6.325226e-01, -6.276326e-01, -6.227664e-01, -6.179238e-01,
    -6.131045e-01, -6.083083e-01, -6.035350e-01, -5.987844e-01, -5.940563e-01, -5.893504e-01, -5.846665e-01, -5.800045e-01, -5.753641e-01,
    -5.707452e-01, -5.661475e-01, -5.615708e-01, -5.570150e-01, -5.524799e-01, -5.479652e-01, -5.434708e-01, -5.389965e-01, -5.345422e-01,
    -5.301076e-01, -5.256925e-01, -5.212969e-01, -5.169205e-01, -5.125632e-01, -5.082248e-01, -5.039052e-01, -4.996041e-01, -4.953214e-01,
    -4.910570e-01, -4.868107e-01, -4.825824e-01, -4.783719e-01, -4.741790e-01, -4.700036e-01, -4.658456e-01, -4.617048e-01, -4.575811e-01,
    -4.534743e-01, -4.493843e-01, -4.453110e-01, -4.412542e-01, -4.372138e-01, -4.331897e-01, -4.291816e-01, -4.251896e-01, -4.212135e-01,
    -4.172531e-01, -4.133083e-01, -4.093790e-01, -4.054651e-01, -4.015665e-01, -3.976830e-01, -3.938145e-01, -3.899609e-01, -3.861221e-01,
    -3.822980e-01, -3.784885e-01, -3.746934e-01, -3.709127e-01, -3.671462e-01, -3.633939e-01, -3.596556e-01, -3.559312e-01, -3.522206e-01,
    -3.485237e-01, -3.448405e-01, -3.411708e-01, -3.375144e-01, -3.338715e-01, -3.302417e-01, -3.266250e-01, -3.230214e-01, -3.194308e-01,
    -3.158529e-01, -3.122879e-01, -3.087355e-01, -3.051957e-01, -3.016683e-01, -2.981534e-01, -2.946507e-01, -2.911603e-01, -2.876821e-01,
    -2.842159e-01, -2.807616e-01, -2.773193e-01, -2.738888e-01, -2.704699e-01, -2.670628e-01, -2.636672e-01, -2.602831e-01, -2.569104e-01,
    -2.535491e-01, -2.501990e-01, -2.468601e-01, -2.435323e-01, -2.402155e-01, -2.369097e-01, -2.336149e-01, -2.303308e-01, -2.270575e-01,
    -2.237948e-01, -2.205428e-01, -2.173013e-01, -2.140703e-01, -2.108496e-01, -2.076394e-01, -2.044394e-01, -2.012496e-01, -1.980699e-01,
    -1.949003e-01, -1.917408e-01, -1.885912e-01, -1.854514e-01, -1.823216e-01, -1.792014e-01, -1.760910e-01, -1.729902e-01, -1.698990e-01,
    -1.668174e-01, -1.637452e-01, -1.606824e-01, -1.576289e-01, -1.545848e-01, -1.515499e-01, -1.485242e-01, -1.455076e-01, -1.425001e-01,
    -1.395016e-01, -1.365120e-01, -1.335314e-01, -1.305596e-01, -1.275967e-01, -1.246424e-01, -1.216969e-01, -1.187601e-01, -1.158318e-01,
    -1.129121e-01, -1.100009e-01, -1.070981e-01, -1.042038e-01, -1.013178e-01, -9.844007e-02, -9.557063e-02, -9.270940e-02, -8.985633e-02,
    -8.701138e-02, -8.417450e-02, -8.134564e-02, -7.852476e-02, -7.571182e-02, -7.290677e-02, -7.010957e-02, -6.732016e-02, -6.453852e-02,
    -6.176459e-02, -5.899834e-02, -5.623972e-02, -5.348868e-02, -5.074520e-02, -4.800922e-02, -4.528070e-02, -4.255961e-02, -3.984591e-02,
    -3.713955e-02, -3.444049e-02, -3.174870e-02, -2.906413e-02, -2.638676e-02, -2.371653e-02, -2.105341e-02, -1.839737e-02, -1.574836e-02,
    -1.310635e-02, -1.047130e-02, -7.843177e-03, -5.221944e-03, -2.607563e-03, -5.773160e-15, 2.600782e-03, 5.194817e-03, 7.782140e-03,
    1.036279e-02,
    1.293679e-02, 1.550419e-02, 1.806501e-02, 2.061929e-02, 2.316706e-02, 2.570836e-02, 2.824321e-02, 3.077166e-02, 3.329373e-02, 3.580945e-02,
    3.831886e-02, 4.082199e-02, 4.331887e-02, 4.580954e-02, 4.829401e-02, 5.077233e-02, 5.324451e-02, 5.571061e-02, 5.817063e-02, 6.062462e-02,
    6.307260e-02, 6.551461e-02, 6.795066e-02, 7.038080e-02, 7.280504e-02, 7.522342e-02, 7.763597e-02, 8.004271e-02, 8.244367e-02, 8.483888e-02,
    8.722837e-02, 8.961216e-02, 9.199028e-02, 9.436276e-02, 9.672963e-02, 9.909090e-02, 1.014466e-01, 1.037968e-01, 1.061415e-01, 1.084806e-01,
    1.108144e-01, 1.131427e-01, 1.154655e-01, 1.177830e-01, 1.200952e-01, 1.224020e-01, 1.247035e-01, 1.269997e-01, 1.292906e-01, 1.315764e-01,
    1.338569e-01, 1.361322e-01, 1.384023e-01, 1.406673e-01, 1.429272e-01, 1.451820e-01, 1.474317e-01, 1.496764e-01, 1.519160e-01, 1.541507e-01,
    1.563803e-01, 1.586050e-01, 1.608248e-01, 1.630396e-01, 1.652496e-01, 1.674546e-01, 1.696549e-01, 1.718503e-01, 1.740408e-01, 1.762266e-01,
    1.784077e-01, 1.805839e-01, 1.827555e-01, 1.849223e-01, 1.870845e-01, 1.892420e-01, 1.913949e-01, 1.935431e-01, 1.956867e-01, 1.978257e-01,
    1.999602e-01, 2.020901e-01, 2.042155e-01, 2.063364e-01, 2.084528e-01, 2.105648e-01, 2.126723e-01, 2.147753e-01, 2.168739e-01, 2.189682e-01,
    2.210580e-01, 2.231436e-01, 2.252247e-01, 2.273016e-01, 2.293741e-01, 2.314424e-01, 2.335063e-01, 2.355661e-01, 2.376216e-01, 2.396729e-01,
    2.417199e-01, 2.437628e-01, 2.458016e-01, 2.478362e-01, 2.498666e-01, 2.518930e-01, 2.539152e-01, 2.559334e-01, 2.579475e-01, 2.599575e-01,
    2.619635e-01, 2.639655e-01, 2.659635e-01, 2.679576e-01, 2.699476e-01, 2.719337e-01, 2.739159e-01, 2.758941e-01, 2.778685e-01, 2.798389e-01,
    2.818055e-01, 2.837682e-01, 2.857270e-01, 2.876821e-01, 2.896333e-01, 2.915807e-01, 2.935243e-01, 2.954642e-01, 2.974003e-01, 2.993327e-01,
    3.012613e-01, 3.031863e-01, 3.051075e-01, 3.070250e-01, 3.089389e-01, 3.108491e-01, 3.127557e-01, 3.146587e-01, 3.165580e-01, 3.184537e-01,
    3.203459e-01, 3.222345e-01, 3.241195e-01, 3.260009e-01, 3.278789e-01, 3.297533e-01, 3.316242e-01, 3.334916e-01, 3.353555e-01, 3.372160e-01,
    3.390730e-01, 3.409266e-01, 3.427767e-01, 3.446234e-01, 3.464668e-01, 3.483067e-01, 3.501432e-01, 3.519764e-01, 3.538062e-01, 3.556327e-01,
    3.574559e-01, 3.592757e-01, 3.610923e-01, 3.629055e-01, 3.647154e-01, 3.665221e-01, 3.683256e-01, 3.701257e-01, 3.719227e-01, 3.737164e-01,
    3.755069e-01, 3.772942e-01, 3.790784e-01, 3.808593e-01, 3.826371e-01, 3.844117e-01, 3.861832e-01, 3.879515e-01, 3.897168e-01, 3.914789e-01,
    3.932379e-01, 3.949938e-01, 3.967467e-01, 3.984964e-01, 4.002432e-01, 4.019868e-01, 4.037275e-01, 4.054651e-01, 4.071997e-01, 4.089313e-01,
    4.106599e-01, 4.123856e-01, 4.141082e-01, 4.158279e-01, 4.175446e-01, 4.192584e-01, 4.209693e-01, 4.226772e-01, 4.243823e-01, 4.260844e-01,
    4.277836e-01, 4.294800e-01, 4.311735e-01, 4.328641e-01, 4.345518e-01, 4.362368e-01, 4.379189e-01, 4.395981e-01, 4.412746e-01, 4.429482e-01,
    4.446190e-01, 4.462871e-01, 4.479524e-01, 4.496149e-01, 4.512746e-01, 4.529316e-01, 4.545859e-01, 4.562374e-01, 4.578862e-01, 4.595323e-01,
    4.611757e-01, 4.628164e-01, 4.644544e-01, 4.660897e-01, 4.677224e-01, 4.693524e-01, 4.709797e-01, 4.726044e-01, 4.742265e-01, 4.758459e-01,
    4.774627e-01, 4.790769e-01, 4.806885e-01, 4.822975e-01, 4.839040e-01, 4.855078e-01, 4.871091e-01, 4.887078e-01, 4.903040e-01, 4.918976e-01,
    4.934887e-01, 4.950773e-01, 4.966633e-01, 4.982468e-01, 4.998279e-01, 5.014064e-01, 5.029824e-01, 5.045560e-01, 5.061271e-01, 5.076957e-01,
    5.092619e-01, 5.108256e-01, 5.123869e-01, 5.139458e-01, 5.155022e-01, 5.170562e-01, 5.186078e-01, 5.201570e-01, 5.217037e-01, 5.232481e-01,
    5.247902e-01, 5.263298e-01, 5.278671e-01, 5.294020e-01, 5.309346e-01, 5.324648e-01, 5.339927e-01, 5.355182e-01, 5.370415e-01, 5.385624e-01,
    5.400810e-01, 5.415973e-01, 5.431113e-01, 5.446230e-01, 5.461324e-01, 5.476396e-01, 5.491445e-01, 5.506471e-01, 5.521475e-01, 5.536456e-01,
    5.551415e-01, 5.566352e-01, 5.581266e-01, 5.596158e-01, 5.611028e-01, 5.625876e-01, 5.640701e-01, 5.655505e-01, 5.670287e-01, 5.685047e-01,
    5.699786e-01, 5.714502e-01, 5.729198e-01, 5.743871e-01, 5.758523e-01, 5.773154e-01, 5.787763e-01, 5.802351e-01, 5.816917e-01, 5.831463e-01,
    5.845987e-01, 5.860490e-01, 5.874973e-01, 5.889434e-01, 5.903874e-01, 5.918294e-01, 5.932693e-01, 5.947071e-01, 5.961429e-01, 5.975766e-01,
    5.990082e-01, 6.004378e-01, 6.018653e-01, 6.032909e-01, 6.047143e-01, 6.061358e-01, 6.075553e-01, 6.089727e-01, 6.103881e-01, 6.118015e-01,
    6.132130e-01, 6.146224e-01, 6.160299e-01, 6.174354e-01, 6.188389e-01, 6.202404e-01, 6.216400e-01, 6.230376e-01, 6.244333e-01, 6.258270e-01,
    6.272188e-01, 6.286087e-01, 6.299966e-01, 6.313826e-01, 6.327667e-01, 6.341488e-01, 6.355291e-01, 6.369075e-01, 6.382839e-01, 6.396585e-01,
    6.410312e-01, 6.424020e-01, 6.437709e-01, 6.451380e-01, 6.465031e-01, 6.478665e-01, 6.492279e-01, 6.505876e-01, 6.519453e-01, 6.533013e-01,
    6.546554e-01, 6.560076e-01, 6.573581e-01, 6.587067e-01, 6.600535e-01, 6.613985e-01, 6.627417e-01, 6.640830e-01, 6.654226e-01, 6.667604e-01,
    6.680964e-01, 6.694307e-01, 6.707631e-01, 6.720938e-01, 6.734227e-01, 6.747498e-01, 6.760752e-01, 6.773988e-01, 6.787207e-01, 6.800408e-01,
    6.813592e-01, 6.826759e-01, 6.839908e-01, 6.853040e-01, 6.866155e-01, 6.879252e-01, 6.892333e-01, 6.905396e-01, 6.918442e-01, 6.931472e-01
};

//   this guy does linear interpolation, it's fun
double crm_log(double x)
{
    double r = 0.0, g;
    int i;

    while (x >= 2.0)
    {
        r += log_lookup_table[768];         //this is log(2)
        x /= 2.0;
    }
    i = (int)(x * 384.0);
    g = x - ((double)i) / 384.0;
    CRM_ASSERT(i < 768);
    CRM_ASSERT(WIDTHOF(log_lookup_table) == 768);
    r += (1.0 - g) * log_lookup_table[i] + g * log_lookup_table[i + 1];
    return r;
}

#define ONE_OVER_SQRT_2PI 0.3989422804014327

double norm_pdf(double x)
{
    return ONE_OVER_SQRT_2PI *exp(-0.5 *x *x);
}

//  this guy makes it so x = 0 yields 1, this is just for when you
//  want normal shaped curves

double normalized_gauss(double x, double s)
{
    return exp(-0.5 * x * x / (s * s));
}

double crm_frand(void)
{
    return (double)rand() / (double)RAND_MAX;
}

void print_histogram_float(float *f, int n, int n_buckets)
{
    int *buckets, i;
    double min, max, s;

    buckets = calloc(n_buckets, sizeof(buckets[0]));
    min = max = f[0];
    for (i = 1; i < n; i++)
        if (f[i] > max)
            max = f[i];
        else if (f[i] < min)
            min = f[i];
    s = (n_buckets - 0.01) / (max - min);
    for (i = 0; i < n_buckets; i++)
        buckets[i] = 0;
    for (i = 0; i < n; i++)
        buckets[(int)(s * (f[i] - min))]++;
    fprintf(stderr, "min: %0.4f, max: %0.4f\n", min, max);
    for (i = 0; i < n_buckets; i++)
    {
        fprintf(stderr, "( %0.4f - %0.4f ): %d\n",
                i / s + min, i / s + min + 1.0 / s, buckets[i]);
    }
    free(buckets);
}

