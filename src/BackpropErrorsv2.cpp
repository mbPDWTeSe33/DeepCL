// Copyright Hugh Perkins 2014 hughperkins at gmail
//
// This Source Code Form is subject to the terms of the Mozilla Public License, 
// v. 2.0. If a copy of the MPL was not distributed with this file, You can 
// obtain one at http://mozilla.org/MPL/2.0/.

#include <algorithm>

#include "StatefulTimer.h"
#include "stringhelper.h"

#include "BackpropErrorsv2Cpu.h"
#include "BackpropErrorsv2Naive.h"
#include "BackpropErrorsv2Cached.h"

#include "BackpropErrorsv2.h"

using namespace std;

#undef STATIC
#define STATIC 

#undef VIRTUAL
#define VIRTUAL 

STATIC BackpropErrorsv2 *BackpropErrorsv2::instance(OpenCLHelper *cl, LayerDimensions dim ) {
    if( ( dim.inputImageSize - dim.filterSize > 6 ) && square( dim.inputImageSize ) <= cl->getMaxWorkgroupSize() ) {
//        return new BackpropErrorsv2Naive( cl, dim );
        return new BackpropErrorsv2Cached( cl, dim );
    } else {
        return new BackpropErrorsv2Naive( cl, dim );
    }
}
STATIC BackpropErrorsv2 *BackpropErrorsv2::instanceForTest(OpenCLHelper *cl, LayerDimensions layerDimensions ) {
    return new BackpropErrorsv2Naive( cl, layerDimensions );
}
STATIC BackpropErrorsv2 *BackpropErrorsv2::instanceSpecific( int idx, OpenCLHelper *cl, LayerDimensions layerDimensions ) {
    if( idx == 0 ) {
        return new BackpropErrorsv2Cpu( cl, layerDimensions );
    }
    if( idx == 1 ) {
        return new BackpropErrorsv2Naive( cl, layerDimensions );
    }
    if( idx == 2 ) {
        return new BackpropErrorsv2Cached( cl, layerDimensions );
    }
    throw std::runtime_error("backproperrorsv2::isntancespecifc, index not known: " + toString( idx ) );
}
BackpropErrorsv2::BackpropErrorsv2( OpenCLHelper *cl, LayerDimensions layerDimensions ) :
        cl( cl ),
        dim( layerDimensions ) {
}
VIRTUAL float * BackpropErrorsv2::backward( int batchSize, float *inputData, float *errors, float *filters ) {
    StatefulTimer::timeCheck("BackpropErrorsv2::backprop begin");

    CLWrapper *inputDataWrapper = cl->wrap( batchSize * dim.inputCubeSize, inputData );
    inputDataWrapper->copyToDevice();

    CLWrapper *gradOutputWrapper = cl->wrap( batchSize * dim.outputCubeSize, errors );
    gradOutputWrapper->copyToDevice();

    int weightsSize = dim.filtersSize;
    CLWrapper *weightsWrapper = cl->wrap( weightsSize, filters );
    weightsWrapper->copyToDevice();

//    CLWrapper *biasWeightsWrapper = 0;
//    if( dim.biased ) {
//        int biasWeightsWrapperSize = dim.numFilters;
//        biasWeightsWrapper = cl->wrap( biasWeightsWrapperSize, biases );
//        biasWeightsWrapper->copyToDevice();
//    }

    int outputDataSize = batchSize * dim.inputCubeSize;
//    cout << " batchsize " << batchSize << " " << dim << endl;
    int allocatedOutputSize = std::max(5000, outputDataSize );
    float *gradInput = new float[allocatedOutputSize];
    CLWrapper *gradInputWrapper = cl->wrap( allocatedOutputSize, gradInput );

    StatefulTimer::timeCheck("BackpropErrorsv2::backprop after copied to device");
    backward( batchSize, inputDataWrapper, gradOutputWrapper, weightsWrapper, gradInputWrapper );
    StatefulTimer::timeCheck("BackpropErrorsv2::backprop after call backprop");
    gradInputWrapper->copyToHost();
    StatefulTimer::timeCheck("BackpropErrorsv2::backprop after copytohost");

    delete gradInputWrapper;
    delete gradOutputWrapper;
    delete weightsWrapper;
    delete inputDataWrapper;
//    if( dim.biased ) {
//        delete biasWeightsWrapper;
//    }

    return gradInput;
}

