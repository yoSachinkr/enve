// enve - 2D animations software
// Copyright (C) 2016-2020 Maurycy Liebner

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "zipfilesaver.h"

ZipFileSaver::ZipFileSaver() {}

void ZipFileSaver::setZipPath(const QString &path) {
    mZip.setZipName(path);
    if(!mZip.open(QuaZip::mdCreate))
        RuntimeThrow("Could not create " + path);
    mFile.setZip(&mZip);
}

void ZipFileSaver::setIoDevice(QIODevice * const src) {
    mZip.setIoDevice(src);
    if(!mZip.open(QuaZip::mdCreate))
        RuntimeThrow("Could not open QIODevice");
    mFile.setZip(&mZip);
}

void ZipFileSaver::process(const QString &file, const Processor &func,
                           const bool compress) {
    if(!mFile.open(QIODevice::WriteOnly, QuaZipNewInfo(file),
                   NULL, compress ? Z_DEFLATED : 0)) {
        RuntimeThrow("Could not open " + file + " in " + mZip.getZipName());
    }
    try {
        func(&mFile);
    } catch(...) {
        mFile.close();
        RuntimeThrow("Could not write " + file + " to " + mZip.getZipName());
    }
    mFile.close();
}

void ZipFileSaver::processText(const QString& file, const TextProcessor& func,
                               const bool compress) {
    process(file, [&func](QIODevice* const dst) {
        QTextStream stream(dst);
        func(stream);
    }, compress);
}
