#include "tilesdata.h"
#include "mainwindow.h"
#include "canvaswindow.h"
#include "tile.h"
#include "memoryhandler.h"

TilesData::TilesData(const ushort &width,
                     const ushort &height,
                     const bool &paintInOtherThread) :
    MinimalCacheContainer(false) {
    mPaintInOtherThread = paintInOtherThread;
    setSize(width, height);
}

TilesData::~TilesData() {
    if(mTiles != NULL) {
        for(int i = 0; i < mNTileRows; i++) {
            for(int j = 0; j < mNTileCols; j++) {
                delete mTiles[i][j];
            }
            delete[] mTiles[i];
        }
        delete[] mTiles;
    }
    if(mTmpFile != NULL) {
        delete mTmpFile;
    }
}

void TilesData::duplicateTilesContentFrom(Tile ***src) {
    for(int i = 0; i < mNTileCols; i++) {
        for(int j = 0; j < mNTileRows; j++) {
            mTiles[j][i]->duplicateFrom(src[j][i]);
        }
    }
}

void TilesData::clearTiles() {
    for(int i = 0; i < mNTileCols; i++) {
        for(int j = 0; j < mNTileRows; j++) {
            mTiles[j][i]->clear();
        }
    }
}

void TilesData::clearTmp() {
    for(int i = 0; i < mNTileCols; i++) {
        for(int j = 0; j < mNTileRows; j++) {
            mTiles[j][i]->clearTmp();
        }
    }
}

void TilesData::getTileDrawers(QList<TileSkDrawer *> *tileDrawers) {
    for(int i = 0; i < mNTileCols; i++) {
        for(int j = 0; j < mNTileRows; j++) {
            tileDrawers->append(mTiles[j][i]->getTexTileDrawer());
        }
    }
}

void TilesData::getTileDrawers(QList<TileSkDrawer *> *tileDrawers,
                               const int &alpha) {
    for(int i = 0; i < mNTileCols; i++) {
        for(int j = 0; j < mNTileRows; j++) {
            TileSkDrawer *tileDrawer =
                    mTiles[j][i]->getTexTileDrawer();
            tileDrawer->alpha = alpha;
            tileDrawers->append(tileDrawer);
        }
    }
}

void TilesData::saveToTmp() {
    for(int i = 0; i < mNTileCols; i++) {
        for(int j = 0; j < mNTileRows; j++) {
            mTiles[j][i]->saveToTmp();
        }
    }
}

void TilesData::drawSk(SkCanvas *canvas, SkPaint *paint) {
    for(int i = 0; i < mNTileCols; i++) {
        for(int j = 0; j < mNTileRows; j++) {
            mTiles[j][i]->drawSk(canvas, paint);
        }
    }
}

void TilesData::move(const ushort &xT, const ushort &yT) {
    if(mNoDataInMemory && !mDataStoredInTmpFile) return;
    // !!! handle differently when stored in tmp file
    int dTileX0T =  -qCeil(xT*1./TILE_DIM);
    int dTileY0T = -qCeil(yT*1./TILE_DIM);
    int x0T = (xT > 0) ? (TILE_DIM - xT%TILE_DIM) : xT%TILE_DIM;
    int y0T = (yT > 0) ? (TILE_DIM - yT%TILE_DIM) : yT%TILE_DIM;
    int width0T = TILE_DIM - x0T;
    int height0T = TILE_DIM - y0T;
// x1 and y1 always 0
    int width1T = TILE_DIM - width0T;
    int height1T = TILE_DIM - height0T;

    int iFirst, iLast, iInc;
    int jFirst, jLast, jInc;

    if(xT > 0) {
        iFirst = mNTileCols - 1;
        iLast = 0;
        iInc = -1;
    } else {
        iFirst = 0;
        iLast = mNTileCols - 1;
        iInc = 1;
    }

    if(yT > 0) {
        jFirst = mNTileRows - 1;
        jLast = 0;
        jInc = -1;
    } else {
        jFirst = 0;
        jLast = mNTileRows - 1;
        jInc = 1;
    }

    int i = iFirst;
    while(true) {
        int j = jFirst;
        while(true) {
            Tile *tileT = mTiles[j][i];
            int tileX0T = i + dTileX0T;
            int tileY0T = j + dTileY0T;
            int tileX1T = tileX0T + 1;
            int tileY1T = tileY0T + 1;
            if(tileX0T >= mNTileCols ||
               tileY0T >= mNTileRows ||
               tileX0T < 0 || tileY0T < 0) {
                tileT->clearData(0, 0,
                                 width0T, height0T);
            } else {
                Tile *srcTileT = mTiles[tileY0T][tileX0T];
                tileT->replaceData(x0T, y0T,
                                   0, 0,
                                   width0T, height0T,
                                   srcTileT);
            }
            if(tileX1T >= mNTileCols ||
               tileY0T >= mNTileRows||
               tileX1T < 0 || tileY0T < 0) {
                tileT->clearData(width0T, 0,
                                 width1T, height0T);
            } else {
                Tile *srcTileT = mTiles[tileY0T][tileX1T];
                tileT->replaceData(0, y0T,
                                   width0T, 0,
                                   width1T, height0T,
                                   srcTileT);
            }
            if(tileX0T >= mNTileCols ||
               tileY1T >= mNTileRows ||
               tileX0T < 0 || tileY1T < 0) {
                tileT->clearData(0, height0T,
                                 width0T, height1T);
            } else {
                Tile *srcTileT = mTiles[tileY1T][tileX0T];
                tileT->replaceData(x0T, 0,
                                   0, height0T,
                                   width0T, height1T,
                                   srcTileT);
            }
            if(tileX1T >= mNTileCols ||
               tileY1T >= mNTileRows ||
               tileX1T < 0 || tileY1T < 0) {
                tileT->clearData(width0T, height0T,
                                 width1T, height1T);
            } else {
                Tile *srcTileT = mTiles[tileY1T][tileX1T];
                tileT->replaceData(0, 0,
                                   width0T, height0T,
                                   width1T, height1T,
                                   srcTileT);
            }
            if(j == jLast) break;
            j += jInc;
        }
        if(i == iLast) break;
        i += iInc;
    }
}

void TilesData::setSize(const ushort &width_t, const ushort &height_t) {
    if(mWidth == width_t && mHeight == height_t) return;
    ushort n_tile_cols_t = ceil(width_t/(qreal)TILE_DIM);
    ushort n_tile_rows_t = ceil(height_t/(qreal)TILE_DIM);
    ushort last_row_height = height_t%TILE_DIM;
    ushort last_column_width = width_t%TILE_DIM;
    resizeTiles(n_tile_cols_t,
                n_tile_rows_t,
                last_column_width,
                last_row_height);

    mWidth = width_t;
    mHeight = height_t;
    mLastRowHeight = last_row_height;
    mLastColumnWidth = last_column_width;
    mNTileRows = n_tile_rows_t;
    mNTileCols = n_tile_cols_t;
}

Tile ***TilesData::getData() {
    return mTiles;
}

void TilesData::setCurrentlyUsed(const bool &used) {
    if(mCurrentlyUsed == used) return;
    mCurrentlyUsed = used;
    if(used) {
        MemoryHandler::getInstance()->removeContainer(this);
        if(mNoDataInMemory) {
            if(mDataStoredInTmpFile) {
                loadDataFromTmpFile();
            } else {
                initializeEmptyTilesData();
            }
        }
    } else if(!mNoDataInMemory) {
        MemoryHandler::getInstance()->addContainer(this);
    }
}

void TilesData::initializeEmptyTilesData() {
    mNoDataInMemory = false;
    for(int i = 0; i < mNTileCols; i++) {
        for(int j = 0; j < mNTileRows; j++) {
            mTiles[j][i]->initializeEmptyTileData();
        }
    }
}

void TilesData::dataChanged() {
    if(mDataStoredInTmpFile) {
        if(mTmpFile != NULL) {
            delete mTmpFile;
            mTmpFile = NULL;
        }
        mDataStoredInTmpFile = false;
    }
}

int TilesData::getByteCount() {
    return mNTileCols*mNTileCols*TILE_DIM*TILE_DIM*4*sizeof(uchar);
}

void TilesData::afterSavedToTmpFile() {
    mDataStoredInTmpFile = true;
}

bool TilesData::cacheAndFree() {
    if(mNoDataInMemory) return false;
    mNoDataInMemory = true;
    if(!mDataStoredInTmpFile) {
        _writeTilesDataToTmp();
        afterSavedToTmpFile();
//        MainWindow::getInstance()->getCanvasWindow()->
//                addUpdatableAwaitingUpdate(
//                    new SaveTilesDataToTmpFileExec(this));
    }
    return true;
}

bool TilesData::freeAndRemove() {
    return false;
}

void TilesData::_writeTilesDataToTmp() {
    if(mTmpFile != NULL) {
        delete mTmpFile;
    }
    mTmpFile = new QTemporaryFile();
    if(mTmpFile->open()) {
        writeTilesData(mTmpFile);
        mTmpFile->close();
    }

    for(int i = 0; i < mNTileCols; i++) {
        for(int j = 0; j < mNTileRows; j++) {
            mTiles[j][i]->clearTileData();
        }
    }
}

void TilesData::_readTilesDataFromTmp() {
    for(int i = 0; i < mNTileCols; i++) {
        for(int j = 0; j < mNTileRows; j++) {
            mTiles[j][i]->initializeTileData();
        }
    }

    if(mTmpFile->open()) {
        readTilesData(mTmpFile);
        mTmpFile->close();
    }
}

void TilesData::loadDataFromTmpFile() {
//    MainWindow::getInstance()->getCanvasWindow()->
//            addUpdatableAwaitingUpdate(
//                new LoadTilesDataFromTmpFileExec(this));
    _readTilesDataFromTmp();
    afterLoadedFromTmpFile();
}

void TilesData::afterLoadedFromTmpFile() {
    mNoDataInMemory = false;
}

void TilesData::resizeTiles(const ushort &nTileCols,
                            const ushort &nTilesRows,
                            const ushort &lastColumnWidth,
                            const ushort &lastRowHeight) {
    Tile ***tiles_t = new Tile**[nTilesRows];

    for(ushort rw = 0; rw < nTilesRows; rw++) {
        tiles_t[rw] = new Tile*[nTileCols];
        ushort first_new_col_in_row = 0;
        if(rw < mNTileRows) {
            first_new_col_in_row = mNTileCols;
            for(ushort cl = 0; cl < mNTileCols; cl++) {
                Tile *tile_t = mTiles[rw][cl];
                if(cl < nTileCols) {
                    tile_t->resetTileSize();
                    tiles_t[rw][cl] = tile_t;
                } else {
                    delete tile_t;
                }
            }
            delete[] mTiles[rw];
        }

        for(ushort cl = first_new_col_in_row; cl < nTileCols; cl++) {
            Tile *newTile = new Tile(cl*TILE_DIM, rw*TILE_DIM,
                                       mPaintInOtherThread);
            if(!mNoDataInMemory) {
                newTile->initializeEmptyTileData();
            }
            tiles_t[rw][cl] = newTile;
        }
    }

    for(int rw = nTilesRows; rw < mNTileRows; rw++) {
        for(ushort cl = 0; cl < mNTileCols; cl++) {
            Tile *tile_t = mTiles[rw][cl];
            delete tile_t;
        }
        delete[] mTiles[rw];
    }

    if(mTiles != NULL) {
        delete[] mTiles;
    }
    if(lastRowHeight != 0) {
        for(int i = 0; i < nTileCols; i++) {
            tiles_t[nTilesRows - 1][i]->setTileHeight(lastRowHeight);
        }
    }
    if(lastColumnWidth != 0) {
        for(int j = 0; j < nTilesRows; j++) {
            tiles_t[j][nTileCols - 1]->setTileWidth(lastColumnWidth);
        }
    }
    mTiles = tiles_t;
}
