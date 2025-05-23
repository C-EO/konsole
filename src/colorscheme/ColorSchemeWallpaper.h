/*
    This source file is part of Konsole, a terminal emulator.

    SPDX-FileCopyrightText: 2007-2008 Robert Knight <robertknight@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef COLORSCHEMEWALLPAPER_H
#define COLORSCHEMEWALLPAPER_H
// STD
#include <memory>

// Qt
#include <QMetaType>
#include <QMovie>
#include <QPointF>
#include <QSharedData>

// Konsole
#include "../characters/CharacterColor.h"

class QPixmap;
class QPainter;

namespace Konsole
{
/**
 * This class holds the wallpaper pixmap associated with a color scheme.
 * The wallpaper object is shared between multiple TerminalDisplay.
 */
class ColorSchemeWallpaper : public QSharedData
{
public:
    enum FillStyle {
        Tile = 0,
        Stretch,
        Crop,
        Adapt,
        NoScaling,
    };
    Q_ENUM(FillStyle)

    enum FlipType {
        NoFlip = 0,
        Horizontal,
        Vertical,
        Both,
    };
    Q_ENUM(FlipType)

    typedef QExplicitlySharedDataPointer<ColorSchemeWallpaper> Ptr;

    explicit ColorSchemeWallpaper(const QString &path,
                                  const ColorSchemeWallpaper::FillStyle style,
                                  const QPointF &anchor,
                                  const qreal &opacity,
                                  const ColorSchemeWallpaper::FlipType flipType);
    ~ColorSchemeWallpaper();

    void load();

    /** Returns true if wallpaper available and drawn */
    bool draw(QPainter &painter, const QRect& rect, qreal bgColorOpacity, const QColor &backgroundColor);

    bool isNull() const;

    QString path() const;

    FillStyle style() const;

    QPointF anchor() const;

    qreal opacity() const;

    FlipType flipType() const;

    bool isAnimated() const;

    int getFrameDelay() const;

private:
    Q_GADGET
    Q_DISABLE_COPY(ColorSchemeWallpaper)

    QString _path;
    std::unique_ptr<QPixmap> _picture;
    std::unique_ptr<QMovie> _movie;
    FillStyle _style;
    QPointF _anchor;
    qreal _opacity;
    FlipType _flipType;
    bool _isAnimated;
    int _frameDelay;

    QRectF ScaledRect(const QSize &viewportSize, const QSize &pictureSize, const QRect &rect);
    Qt::AspectRatioMode RatioMode();
    QImage FlipImage(const QImage &image, const ColorSchemeWallpaper::FlipType flipType);
};

}

#endif
