#ifndef LSST_MEAS_EXTENSIONS_ASTROMETRYNET_UTILS_H
#define LSST_MEAS_EXTENSIONS_ASTROMETRYNET_UTILS_H

#include <string>
#include <vector>

#include "lsst/afw/table/Source.h"
#include "lsst/afw/coord.h"
#include "lsst/geom.h"

namespace lsst {
namespace meas {
namespace extensions {
namespace astrometryNet {
namespace detail {

    struct MagColInfo {
        std::string filterName; ///< name of filter
        std::string magCol;     ///< name of magnitude column
        std::string magErrCol;  ///< name of magnitude sigma column

        bool hasErr() const {
            return !magErrCol.empty();
        }
    };


/// RAII manager for astrometry.net indices
///
/// Ensures index files are closed when done, to prevent "Too many open files" errors.
/// See also #2292, #2879.
struct IndexManager {
    index_t* index;
    IndexManager(index_t* ind) : index(ind) {}
    ~IndexManager() {
        // Change once astrometry.net-0.40+ is in...
        /*
          if (index_close_fds(ind)) {
          throw LSST_EXCEPT(lsst::pex::exceptions::IoError,
          "Failed to index_close_fds() an astrometry_net_data file");
          }
        */
        if (index->quads && index->quads->fb && index->quads->fb->fid) {
            _close(index->quads->fb->fid);
        }
        if (index->codekd && index->codekd->tree && index->codekd->tree->io) {
            _close(index->codekd->tree->io);
        }
        if (index->starkd && index->starkd->tree && index->starkd->tree->io) {
            _close(index->starkd->tree->io);
        }
    }
    void _close(FILE* & fid) {
        if (fid) {
            if (fclose(fid)) {
                std::cerr << "Error closing an astrometry_net_data quadfile" << std::endl;
            }
            fid = NULL;
        }
    }
    void _close(void* io) {
        _close(reinterpret_cast<kdtree_fits_t*>(io)->fid);
    }
};

/**
Implementation for index_t::getCatalog method

@param[in] inds  star kd-trees from astrometry.net
@param[in] ctrCoord  center of search region
@param[in] radius  search radius
@param[in] idCol  name of ID column in astrometry.net data
@param[in] magColInfoList  list of information about magnitude columns in astrometry.net data
@param[in] starGalCol  name of "starGal" column (true if object is a star) in astrometry.net data
@param[in] varCol  name of "var" column (true if brightness is variable) in astrometry.net data
@param[in] uniqueIds  if true then only return unique IDs (the first of each seen)

Returned schema:
- id
- coord: ICRS sky position (an lsst::geom::SpherePoint)
- centroid: centroid on some exposure, if relevant (an lsst::geom::Point2D); returned value is not set
- hasCentroid: if true then centroid has been set; returned value is false
- *filterName*_flux: flux in the specified filter (double)
- *filterName*_fluxErr: flux uncertainty in the specified filter (double)
- resolved (if starGalCol specified): true if object is not resolved
- variable (if varCol specified): true if brightness is variable
- photometric: true if not resolved (or starGalCol blank) and not variable (or varCol blank);
    note that if starGalCol and varCol both blank then all objects are claimed to be photometric
*/
lsst::afw::table::SimpleCatalog
getCatalogImpl(
    std::vector<index_t*> inds,
    lsst::geom::SpherePoint const &ctrCoord,
    lsst::geom::Angle const &radius,
    const char* idCol,
    std::vector<MagColInfo> const& magColInfoList,
    const char* starGalCol,
    const char* varCol,
    bool uniqueIds=true);

}}}}}  // lsst::meas::extensions::astrometryNet::detail

#endif // LSST_MEAS_EXTENSIONS_ASTROMETRYNET_UTILS_H
