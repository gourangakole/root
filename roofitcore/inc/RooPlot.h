/*****************************************************************************
 * Project: BaBar detector at the SLAC PEP-II B-factory
 * Package: RooFitCore
 *    File: $Id: RooPlot.rdl,v 1.22 2002/03/07 06:22:22 verkerke Exp $
 * Authors:
 *   DK, David Kirkby, Stanford University, kirkby@hep.stanford.edu
 * History:
 *   30-Nov-2000 DK Created initial version
 *
 * Copyright (C) 2001 Stanford University
 *****************************************************************************/
#ifndef ROO_PLOT
#define ROO_PLOT

#include "TH1.h"
#include "RooFitCore/RooList.hh"
#include "RooFitCore/RooPrintable.hh"

class RooAbsReal;
class RooAbsRealLValue;
class RooArgSet ;
class RooHist;
class RooPlotable;
class TAttLine;
class TAttFill;
class TAttMarker;
class TAttText;
class TClass ;

class RooPlot : public TH1, public RooPrintable {
public:
  RooPlot(const RooAbsReal &var, Double_t xmin, Double_t xmax, Int_t nBins);
  RooPlot(Double_t xmin= 0, Double_t xmax= 1);
  RooPlot(Double_t xmin, Double_t xmax, Double_t ymin, Double_t ymax);
  RooPlot(const RooAbsRealLValue &var1, const RooAbsRealLValue &var2);
  RooPlot(const RooAbsReal &var1, const RooAbsReal &var2,
	  Double_t xmin, Double_t xmax, Double_t ymin, Double_t ymax);
  virtual ~RooPlot();

  // implement the TH1 interface
  virtual Stat_t GetBinContent(Int_t) const;
  virtual Stat_t GetBinContent(Int_t, Int_t) const;
  virtual Stat_t GetBinContent(Int_t, Int_t, Int_t) const;
  virtual void Draw(Option_t *options= 0);

  // container management
  const char* nameOf(Int_t idx) const ;
  TObject *findObject(const char *name, const TClass* clas=0) const;
  Stat_t numItems() const {return _items.GetSize();}

  void addPlotable(RooPlotable *plotable, Option_t *drawOptions= "");
  void addObject(TObject* obj, Option_t* drawOptions= "");
  void addTH1(TH1 *hist, Option_t* drawOptions= "");

  // ascii printing
  virtual void printToStream(ostream& os, PrintOption opt= Standard, TString indent= "") const;
  inline virtual void Print(Option_t *options= 0) const {
    printToStream(defaultStream(),parseOptions(options));
  }

  // data member get/set methods
  inline RooAbsReal *getPlotVar() const { return _plotVarClone; }
  inline Double_t getFitRangeNEvt() const { return _normNumEvts; }
  inline Double_t getFitRangeBinW() const { return _normBinWidth; }
  inline Double_t getPadFactor() const { return _padFactor; }
  inline void setPadFactor(Double_t factor) { if(factor >= 0) _padFactor= factor; }
  void updateNormVars(const RooArgSet &vars);
  const RooArgSet *getNormVars() const { return _normVars; }

  // get attributes of contained objects
  TAttLine *getAttLine(const char *name=0) const;
  TAttFill *getAttFill(const char *name=0) const;
  TAttMarker *getAttMarker(const char *name=0) const;
  TAttText *getAttText(const char *name=0) const;

  // rearrange drawing order of contained objects
  Bool_t drawBefore(const char *before, const char *target);
  Bool_t drawAfter(const char *after, const char *target);

  // get/set drawing options for contained objects
  TString getDrawOptions(const char *name) const;
  Bool_t setDrawOptions(const char *name, TString options);
 
  virtual void SetMaximum(Double_t maximum = -1111) ;
  virtual void SetMinimum(Double_t minimum = -1111) ;

  Double_t chiSquare(const char* pdfname=0, const char* histname=0) const ;

  void initialize();
  TString histName() const ; 
  TString caller(const char *method) const;
  void updateYAxis(Double_t ymin, Double_t ymax, const char *label= "");
  void updateFitRangeNorm(const TH1* hist);
  void updateFitRangeNorm(const RooPlotable* rp);

  RooList _items;            // A list of the items we contain.
  Double_t _padFactor;       // Scale our y-axis to _padFactor of our maximum contents.
  RooAbsReal *_plotVarClone; // A clone of the variable we are plotting.
  RooArgSet *_plotVarSet;    // A list owning the cloned tree nodes of the plotVarClone
  RooArgSet *_normVars;      // Variables that PDF plots should be normalized over
  Double_t _normNumEvts;     // Number of events in histogram (for normalization)
  Double_t _normBinWidth;    // Histogram bin width (for normalization)
  //Double_t _normValue;       // Fit-range normalization to use for plotting PDFs
  TIterator *_iterator;      //! non-persistent

  Double_t _defYmin ;        // Default minimum for Yaxis (as calculated from contents)
  Double_t _defYmax ;        // Default maximum for Yaxis (as calculated from contents)

protected:
  RooPlot(const RooPlot& other); // object cannot be copied

  ClassDef(RooPlot,1)        // Plot frame and container for graphics objects
};

#endif
