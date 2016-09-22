#include "ctrl/TimeLineFocus.h"

using namespace core;

namespace ctrl
{
//-------------------------------------------------------------------------------------------------
TimeLineFocus::SingleFocus::SingleFocus()
    : node()
    , pos()
{
}

bool TimeLineFocus::SingleFocus::isValid() const
{
    return node && !pos.isNull();
}

//-------------------------------------------------------------------------------------------------
TimeLineFocus::TimeLineFocus(
        const QVector<TimeLineRow>& aRows,
        const TimeLineScale& aScale,
        int aMargin)
    : mRows(aRows)
    , mScale(aScale)
    , mFocusLink()
    , mPoint()
    , mRange()
    , mFoundFocus(false)
    , mViewIsChanged(false)
    , mMargin(aMargin)
    , mRadius(5)
{
}

TimeLineFocus::SingleFocus TimeLineFocus::reset(const QPoint& aPoint)
{
    mPoint = aPoint;
    const int beginFrame = mScale.frame((aPoint.x() - 2) - mMargin);
    const int endFrame   = mScale.frame((aPoint.x() + 2) - mMargin);
    mRange.setLeft(beginFrame);
    mRange.setRight(endFrame);
    mRange.setTop(aPoint.y());
    mRange.setBottom(aPoint.y());

    SingleFocus single = updateImpl(true);

    const bool foundFocus = single.isValid();
    mViewIsChanged = (foundFocus != mFoundFocus);
    mFoundFocus = foundFocus;
    return single;
}

bool TimeLineFocus::update(const QPoint& aPoint)
{
    const int frame0 = mScale.frame(mPoint.x() - mMargin);
    const int frame1 = mScale.frame(aPoint.x() - mMargin);

    if (frame0 <= frame1)
    {
        mRange.setLeft(frame0);
        mRange.setRight(frame1);
    }
    else
    {
        mRange.setLeft(frame1);
        mRange.setRight(frame0);
    }

    if (mPoint.y() <= aPoint.y())
    {
        mRange.setTop(mPoint.y());
        mRange.setBottom(aPoint.y());
    }
    else
    {
        mRange.setTop(aPoint.y());
        mRange.setBottom(mPoint.y());
    }

    SingleFocus single = updateImpl(false);

    const bool foundFocus = single.isValid();
    mViewIsChanged = (foundFocus != mFoundFocus);
    mFoundFocus = foundFocus;
    return foundFocus;
}

QRect TimeLineFocus::visualRect() const
{
    QRect box = mRange;
    box.setLeft(mScale.pixelWidth(box.left()) + mMargin);
    box.setRight(mScale.pixelWidth(box.right()) + mMargin);
    return box;
}

QRect TimeLineFocus::boundingRect() const
{
    QRect box = mRange;
    box.setLeft(mScale.pixelWidth(box.left()) + mMargin);
    box.setRight(mScale.pixelWidth(box.right()) + mMargin);
    box.setTop(mRange.top() - mRadius);
    box.setBottom(mRange.bottom() + mRadius);
    return box;
}

void TimeLineFocus::moveBoundingRect(int aAddFrame)
{
    mRange.setLeft(mRange.left() + aAddFrame);
    mRange.setRight(mRange.right() + aAddFrame);
}

bool TimeLineFocus::select(TimeLineEvent& aEvent)
{
    const QRect box = boundingRect();
    bool found = false;

    for (auto line : mRows)
    {
        if (!line.rect.intersects(box)) continue;

        ObjectNode::Iterator nodeItr(line.node);
        while (nodeItr.hasNext())
        {
            ObjectNode* node = nodeItr.next();
            XC_PTR_ASSERT(node->timeLine());
            TimeLine& timeLine = *(node->timeLine());
            const int validNum = timeLine.validTypeCount();
            int validIndex = 0;


            for (int i = 0; i < TimeKeyType_TERM; ++i)
            {
                const TimeLine::MapType& map = timeLine.map((TimeKeyType)i);
                if (map.isEmpty()) continue;

                const float height = line.keyHeight(validIndex, validNum);
                ++validIndex;
                if (height < box.top() || box.bottom() < height) continue;

                auto itr = map.lowerBound(mRange.left());
                while (itr != map.end() && itr.key() <= mRange.right())
                {
                    TimeKey* key = itr.value();
                    if (key)
                    {
                        const int frame = itr.key();
                        aEvent.pushTarget(*node, (TimeKeyType)i, frame);
                        found = true;
                    }
                    ++itr;
                }
            }
            if (!line.closedFolder) break;
        }
    }
    return found;
}

TimeLineFocus::SingleFocus TimeLineFocus::updateImpl(bool aForceSingle)
{
    SingleFocus single;

    mFocusLink.construct();

    const QRect box = boundingRect();

    for (auto line : mRows)
    {
        if (!line.rect.intersects(box)) continue;

        ObjectNode::Iterator nodeItr(line.node);
        while (nodeItr.hasNext())
        {
            ObjectNode* node = nodeItr.next();
            XC_PTR_ASSERT(node->timeLine());
            TimeLine& timeLine = *(node->timeLine());
            const int validNum = timeLine.validTypeCount();
            int validIndex = 0;

            for (int i = 0; i < TimeKeyType_TERM; ++i)
            {
                const TimeLine::MapType& map = timeLine.map((TimeKeyType)i);
                if (map.isEmpty()) continue;

                const float height = line.keyHeight(validIndex, validNum);
                ++validIndex;
                if (height < box.top() || box.bottom() < height) continue;

                auto itr = map.lowerBound(mRange.left());
                while (itr != map.end() && itr.key() <= mRange.right())
                {
                    TimeKey* key = itr.value();
                    if (key)
                    {
                        key->setFocus(*mFocusLink);

                        single.node = node;
                        single.pos.setLine(&timeLine);
                        single.pos.setType((TimeKeyType)i);
                        single.pos.setIndex(itr.key());

                        if (aForceSingle)
                        {
                            return single;
                        }
                    }
                    ++itr;
                }
            }
            if (!line.closedFolder) break;
        }
    }
    return single;
}

bool TimeLineFocus::isInRange(const QPoint& aPoint) const
{
    QRect boundingBox = mRange;
    boundingBox.setLeft(mScale.pixelWidth(boundingBox.left()) + mMargin);
    boundingBox.setRight(mScale.pixelWidth(boundingBox.right()) + mMargin);
    return boundingBox.contains(aPoint);
}

bool TimeLineFocus::hasRange() const
{
    return mRange.left() < mRange.right() && mRange.top() < mRange.bottom();
}

void TimeLineFocus::clear()
{
    mPoint = QPoint();
    mRange = QRect();
    mFocusLink.destruct();
    mFoundFocus = false;
    mViewIsChanged = true;
}

bool TimeLineFocus::viewIsChanged() const
{
    return mViewIsChanged;
}

} // namespace ctrl