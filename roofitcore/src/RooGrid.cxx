/*****************************************************************************
 * Project: BaBar detector at the SLAC PEP-II B-factory
 * Package: RooFitCore
 *    File: $Id: RooGrid.cc,v 1.4 2001/10/08 05:20:16 verkerke Exp $
 * Authors:
 *   DK, David Kirkby, Stanford University, kirkby@hep.stanford.edu
 *   WV, Wouter Verkerke, UC Santa Barbara, verkerke@slac.stanford.edu
 * History:
 *   08-Aug-2001 WV Created initial version
 *
 * Copyright (C) 2001 Stanford University
 *****************************************************************************/

// -- CLASS DESCRIPTION [AUX] --
// RooMCIntegrator implements an adaptive multi-dimensional Monte Carlo
// numerical integration, following the VEGAS algorithm.

#include "RooFitCore/RooGrid.hh"
#include "RooFitCore/RooAbsFunc.hh"
#include "RooFitCore/RooNumber.hh"
#include "RooFitCore/RooRandom.hh"

#include <math.h>
#include <iostream.h>
#include <iomanip.h>

ClassImp(RooGrid)
;

RooGrid::RooGrid(const RooAbsFunc &function)
  : _valid(kTRUE), _xl(0),_xu(0),_delx(0),_xi(0)
{
  // check that the input function is valid
  if(!(_valid= function.isValid())) {
    cout << ClassName() << ": cannot initialize using an invalid function" << endl;
    return;
  }

  // allocate workspace memory
  _dim= function.getDimension();
  _xl= new Double_t[_dim];
  _xu= new Double_t[_dim];
  _delx= new Double_t[_dim];
  _d= new Double_t[_dim*maxBins];
  _xi= new Double_t[_dim*(maxBins+1)];
  _xin= new Double_t[maxBins+1];
  _weight= new Double_t[maxBins];
  if(!_xl || !_xu || !_delx || !_d || !_xi || !_xin || !_weight) {
    cout << ClassName() << ": memory allocation failed" << endl;
    _valid= kFALSE;
    return;
  }

  // initialize the grid
  _valid= initialize(function);
}

RooGrid::~RooGrid() {
  if(_xl)     delete[] _xl;
  if(_xu)     delete[] _xu;
  if(_delx)   delete[] _delx;
  if(_d)      delete[] _d;
  if(_xi)     delete[] _xi;
  if(_xin)    delete[] _xin;
  if(_weight) delete[] _weight;
}

Bool_t RooGrid::initialize(const RooAbsFunc &function) {
  // Calculate and store the grid dimensions and volume using the
  // specified function, and initialize the grid using a single bin.
  // Return kTRUE, or else kFALSE if the range is not valid.

  _vol= 1;
  _bins= 1;
  for(UInt_t index= 0; index < _dim; index++) {
    _xl[index]= function.getMinLimit(index);
    if(RooNumber::isInfinite(_xl[index])) {
      cout << ClassName() << ": lower limit of dimension " << index << " is infinite" << endl;
      return kFALSE;
    }
    _xu[index]= function.getMaxLimit(index);
    if(RooNumber::isInfinite(_xl[index])) {
      cout << ClassName() << ": upper limit of dimension " << index << " is infinite" << endl;
      return kFALSE;
    }
    Double_t dx= _xu[index] - _xl[index];
    if(dx <= 0) {
      cout << ClassName() << ": bad range for dimension " << index << ": [" << _xl[index]
	   << "," << _xu[index] << "]" << endl;
      return kFALSE;
    }
    _delx[index]= dx;
    _vol*= dx;
    coord(0,index) = 0;
    coord(1,index) = 1;
  }
  return kTRUE;
}

void RooGrid::resize(UInt_t bins) {
  // Adjust the subdivision of each axis to give the specified
  // number of bins, using an algorithm that preserves relative
  // bin density. The new binning can be finer or coarser than
  // the original binning.

  // is there anything to do?
  if(bins == _bins) return;
  
  // weight is ratio of bin sizes
  Double_t pts_per_bin = (Double_t) _bins / (Double_t) bins;

  // loop over grid dimensions
  for (UInt_t j = 0; j < _dim; j++) {
    Double_t xold,xnew(0),dw(0);
    Int_t i = 1;
    // loop over bins in this dimension and load _xin[] with new bin edges

    UInt_t k;
    for(k = 1; k <= _bins; k++) {
      dw += 1.0;
      xold = xnew;
      xnew = coord(k,j);      
      while(dw > pts_per_bin) {
	dw -= pts_per_bin;
	newCoord(i++)= xnew - (xnew - xold) * dw;
      }
    }
    // copy the new edges into _xi[j]
    for(k = 1 ; k < bins; k++) {
      coord(k, j) = newCoord(k);
    }
    coord(bins, j) = 1;
  }  
  _bins = bins;
}

void RooGrid::resetValues() {
  // Reset the values associated with each grid cell.

  for(UInt_t i = 0; i < _bins; i++) {
    for (UInt_t j = 0; j < _dim; j++) {
      value(i,j)= 0.0;
    }
  }
}

void RooGrid::generatePoint(const UInt_t box[], Double_t x[], UInt_t bin[], Double_t &vol,
			    Bool_t useQuasiRandom) const {
  // Generate a random vector in the specified box and and store its
  // coordinates in the x[] array provided, the corresponding bin
  // indices in the bin[] array, and the volume of this bin in vol.
  // The box is specified by the array box[] of box integer indices
  // that each range from 0 to getNBoxes()-1.

  vol= 1;

  // generate a vector of quasi-random numbers to use
  if(useQuasiRandom) {
    RooRandom::quasi(_dim,x);
  }
  else {
    RooRandom::uniform(_dim,x);
  }

  // loop over coordinate axes
  for(UInt_t j= 0; j < _dim; ++j) {

    // generate a random point uniformly distributed (in box space)
    // within the box[j]-th box of coordinate axis j.
    Double_t z= ((box[j] + x[j])/_boxes)*_bins;

    // store the bin in which this point lies along the j-th
    // coordinate axis and calculate its width and position y
    // in normalized bin coordinates.
    Int_t k= (Int_t)z;
    bin[j] = k;
    Double_t y, bin_width;
    if(k == 0) {
      bin_width= coord(1,j);
      y= z * bin_width;
    }
    else {
      bin_width= coord(k+1,j) - coord(k,j);
      y= coord(k,j) + (z-k)*bin_width;
    }
    // transform from normalized bin coordinates to x space.
    x[j] = _xl[j] + y*_delx[j];

    // update this bin's calculated volume
    vol *= bin_width;
  }
}

void RooGrid::firstBox(UInt_t box[]) const {
  // Reset the specified array of box indices to refer to the first box
  // in the standard traversal order.

  for(UInt_t i= 0; i < _dim; i++) box[i]= 0;
}

Bool_t RooGrid::nextBox(UInt_t box[]) const {
  // Update the specified array of box indices to refer to the next box
  // in the standard traversal order and return kTRUE, or else return
  // kFALSE if we the indices already refer to the last box.

  // try incrementing each index until we find one that does not roll
  // over, starting from the last index.
  Int_t j(_dim-1);
  while (j >= 0) {
    box[j]= (box[j] + 1) % _boxes;
    if (0 != box[j]) return kTRUE;
    j--;
  }
  // if we get here, then there are no more boxes
  return kFALSE;
}

void RooGrid::printToStream(ostream& os, PrintOption opt, TString indent) const {
  // Print info about this object to the specified stream.

  os << ClassName() << ": volume = " << getVolume() << endl;
  if(opt >= Standard) {
    cout << indent << "  Has " << getDimension() << " dimension(s) each subdivided into "
	 << getNBins() << " bin(s) and sampled with " << _boxes << " box(es)" << endl;
    for(UInt_t index= 0; index < getDimension(); index++) {
      cout << indent << "  (" << index << ") ["
	   << setw(10) << _xl[index] << "," << setw(10) << _xu[index] << "]" << endl;
      if(opt < Verbose) continue;
      for(UInt_t bin= 0; bin < _bins; bin++) {
	cout << indent << "    bin-" << bin << " : x = " << coord(bin,index) << " , y = "
	     << value(bin,index) << endl;
      }
    }
  }
}

void RooGrid::accumulate(const UInt_t bin[], Double_t amount) {
  // Add the specified amount to bin[j] of the 1D histograms associated
  // with each axis j.

  for(UInt_t j = 0; j < _dim; j++) value(bin[j],j) += amount;
}

void RooGrid::refine(Double_t alpha) {
  // Refine the grid using the values that have been accumulated so far.
  // The parameter alpha controls the stiffness of the rebinning and should
  // usually be between 1 (stiffer) and 2 (more flexible). A value of zero
  // prevents any rebinning.

  for (UInt_t j = 0; j < _dim; j++) {

    // smooth this dimension's histogram of grid values and calculate the
    // new sum of the histogram contents as grid_tot_j
    Double_t oldg = value(0,j);
    Double_t newg = value(1,j);
    value(0,j)= (oldg + newg)/2;
    Double_t grid_tot_j = value(0,j);    
    // this loop implements value(i,j) = ( value(i-1,j)+value(i,j)+value(i+1,j) ) / 3

    UInt_t i;
    for (i = 1; i < _bins - 1; i++) {
      Double_t rc = oldg + newg;
      oldg = newg;
      newg = value(i+1,j);
      value(i,j)= (rc + newg)/3;
      grid_tot_j+= value(i,j);
    }
    value(_bins-1,j)= (newg + oldg)/2;
    grid_tot_j+= value(_bins-1,j);

    // calculate the weights for each bin of this dimension's histogram of values
    // and their sum
    Double_t tot_weight(0);
    for (i = 0; i < _bins; i++) {
      _weight[i] = 0;
      if (value(i,j) > 0) {
	oldg = grid_tot_j/value(i,j);
	/* damped change */
	_weight[i] = pow(((oldg-1.0)/oldg/log(oldg)), alpha);
      }
      tot_weight += _weight[i];
    }

    Double_t pts_per_bin = tot_weight / _bins;
    
    Double_t xold;
    Double_t xnew = 0;
    Double_t dw = 0;    

    UInt_t k;
    i = 1;
    for (k = 0; k < _bins; k++) {
      dw += _weight[k];
      xold = xnew;
      xnew = coord(k+1,j);
      
      while(dw > pts_per_bin) {
	dw -= pts_per_bin;
	newCoord(i++) = xnew - (xnew - xold) * dw / _weight[k];
      }
    }
    
    for (k = 1 ; k < _bins ; k++) {
      coord( k, j) = newCoord(k);
    }
    
    coord(_bins, j) = 1;
  }
}
