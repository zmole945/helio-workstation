/*
    This file is part of Helio Workstation.

    Helio is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Helio is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Helio. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

class MidiSequence;
class HybridLassoComponent;
class ProjectTreeItem;
class LongTapController;
class SmartDragController;
class SmoothPanController;
class SmoothZoomController;
class MultiTouchController;
class OverlayShadow;
class HybridRollHeader;
class TriggersTrackMap;
class Transport;
class HybridRollListener;
class WipeSpaceHelper;
class InsertSpaceHelper;
class TimelineWarningMarker;

#include "ComponentFader.h"
#include "AnnotationsTrackMap.h"
#include "AnnotationLargeComponent.h"
#include "TimeSignaturesTrackMap.h"
#include "TimeSignatureLargeComponent.h"
#include "KeySignatureLargeComponent.h"
#include "Playhead.h"
#include "TransportListener.h"
#include "HybridRollEventComponent.h"
#include "ClipboardOwner.h"
#include "LongTapListener.h"
#include "SmoothPanListener.h"
#include "SmoothZoomListener.h"
#include "MultiTouchListener.h"
#include "ProjectListener.h"
#include "Lasso.h"
#include "HybridRollEditMode.h"
#include "AudioMonitor.h"
#include "Serializable.h"

#define HYBRID_ROLL_MAX_BAR_WIDTH (192)
#define HYBRID_ROLL_HEADER_HEIGHT (40)

#define DEFAULT_NUM_BARS 8

#define HYBRID_ROLL_BULK_REPAINT_START \
    this->setVisible(false);

#define HYBRID_ROLL_BULK_REPAINT_END \
    this->setVisible(true);

class HybridRoll :
    public Component,
    public Serializable,
    public LongTapListener,
    public SmoothPanListener,
    public SmoothZoomListener,
    public MultiTouchListener,
    public ProjectListener,
    public ClipboardOwner,
    public LassoSource<SelectableComponent *>,
    protected ChangeListener, // listens to HybridRollEditMode,
    protected TransportListener,
    protected AsyncUpdater, // for async scrolling on transport listener events
    protected HighResolutionTimer, // for smooth scrolling to seek position
    protected Playhead::Listener, // for smooth scrolling to seek position
    protected AudioMonitor::ClippingListener // for displaying clipping indicator components
{
public:
    
    HybridRoll(ProjectTreeItem &project, Viewport &viewport,
        WeakReference<AudioMonitor> audioMonitor,
        bool hasAnnotationsTrack = true,
        bool hasKeySignaturesTrack = true,
        bool hasTimeSignaturesTrack = true);

    ~HybridRoll() override;

    Viewport &getViewport() const noexcept;
    Transport &getTransport() const noexcept;
    ProjectTreeItem &getProject() const noexcept;
    HybridRollEditMode getEditMode() const;

    virtual void selectAll() = 0;
    virtual Rectangle<float> getEventBounds(FloatBoundsComponent *nc) const = 0;
    
    void scrollToSeekPosition();
    float getPositionForNewTimelineEvent() const;
    void insertAnnotationWithinScreen(const String &annotation);
    void insertTimeSignatureWithinScreen(int numerator, int denominator);
    
    //===------------------------------------------------------------------===//
    // Custom maps
    //===------------------------------------------------------------------===//
    
    void addOwnedMap(Component *newTrackMap);
    void removeOwnedMap(Component *existingTrackMap);
    template<typename T> inline T *findOwnedMapOfType() const
    {
        for (int i = 0; i < this->trackMaps.size(); ++i)
        {
            if (T *target = dynamic_cast<T *>(this->trackMaps.getUnchecked(i)))
            {
                return target;
            }
        }
        
        return nullptr;
    }
    
    //===------------------------------------------------------------------===//
    // Modes
    //===------------------------------------------------------------------===//

    bool isInSelectionMode() const;
    bool isInDragMode() const;
    // ...
    
    //===------------------------------------------------------------------===//
    // HybridRoll listeners management
    //===------------------------------------------------------------------===//
    
    void addRollListener(HybridRollListener *listener);
    void removeRollListener(HybridRollListener *listener);
    void removeAllRollListeners();
    
    //===------------------------------------------------------------------===//
    // MultiTouchListener
    //===------------------------------------------------------------------===//

    void multiTouchZoomEvent(const Point<float> &origin, const Point<float> &zoom) override;
    void multiTouchPanEvent(const Point<float> &offset) override;
    void multiTouchCancelZoom() override;
    void multiTouchCancelPan() override;
    Point<float> getMultiTouchOrigin(const Point<float> &from) override;

    //===------------------------------------------------------------------===//
    // SmoothPanListener
    //===------------------------------------------------------------------===//

    void panByOffset(const int offsetX, const int offsetY) override;
    void panProportionally(const float absX, const float absY) override;

    Point<int> getPanOffset() const override;

    //===------------------------------------------------------------------===//
    // SmoothZoomListener
    //===------------------------------------------------------------------===//

    void zoomAbsolute(const Point<float> &zoom) override;
    void zoomRelative(const Point<float> &origin, const Point<float> &factor) override;
    float getZoomFactorX() const override;
    float getZoomFactorY() const override;

    void startSmoothZoom(const Point<float> &origin, const Point<float> &factor);
    void zoomInImpulse();
    void zoomOutImpulse();

    //===------------------------------------------------------------------===//
    // Misc
    //===------------------------------------------------------------------===//
    
    int getXPositionByTransportPosition(double absPosition, double canvasWidth) const;
    double getTransportPositionByXPosition(int xPosition, double canvasWidth) const;

    double getTransportPositionByBeat(float targetBeat) const;
    float getBeatByTransportPosition(double absSeekPosition) const;

    float getBarByXPosition(int xPosition) const;
    int getXPositionByBar(float targetBar) const;

    int getXPositionByBeat(float targetBeat) const;
    float getFloorBeatByXPosition(int x) const;
    float getRoundBeatByXPosition(int x) const;
    
    inline float getLastBar() const noexcept { return this->lastBar; }
    inline float getLastBeat() const noexcept { return this->lastBar * float(NUM_BEATS_IN_BAR); }
    
    inline float getFirstBar() const noexcept { return this->firstBar; }
    inline float getFirstBeat() const noexcept { return this->firstBar * float(NUM_BEATS_IN_BAR); }
    
    void setBarRange(float first, float last);
    inline float getNumBars() const { return this->lastBar - this->firstBar; }
    inline float getNumBeats() const { return this->getNumBars() * NUM_BEATS_IN_BAR; }

    virtual void setBarWidth(const float newBarWidth);
    float getBarWidth() const noexcept { return this->barWidth; }

    inline const Array<float> &getVisibleBars() const noexcept  { return this->visibleBars; }
    inline const Array<float> &getVisibleBeats() const noexcept { return this->visibleBeats; }
    inline const Array<float> &getVisibleSnaps() const noexcept { return this->visibleSnaps; }
    
    bool isUsingAnyAltMode() const;
    void setSpaceDraggingMode(bool dragMode);
    bool isUsingSpaceDraggingMode() const;
    void setAltDrawingMode(bool drawMode);
    bool isUsingAltDrawingMode() const;
    
    void triggerBatchRepaintFor(FloatBoundsComponent *target);

    bool isFollowingPlayhead() const noexcept;
    void startFollowingPlayhead();
    void stopFollowingPlayhead();
    
    //===------------------------------------------------------------------===//
    // LassoSource
    //===------------------------------------------------------------------===//

    virtual void selectEventsInRange(float startBeat,
        float endBeat, bool shouldClearAllOthers) = 0;

    Lasso &getLassoSelection() override;
    void selectEvent(SelectableComponent *event, bool shouldClearAllOthers);
    void deselectEvent(SelectableComponent *event);
    void deselectAll();
    
    HybridLassoComponent *getLasso() const;
    
    //===------------------------------------------------------------------===//
    // ProjectListener
    //===------------------------------------------------------------------===//

    void onChangeMidiEvent(const MidiEvent &oldEvent, const MidiEvent &newEvent) override;
    void onAddMidiEvent(const MidiEvent &event) override;
    void onRemoveMidiEvent(const MidiEvent &event) override;
    void onChangeProjectBeatRange(float firstBeat, float lastBeat) override;
    void onChangeViewBeatRange(float firstBeat, float lastBeat) override;

    //===------------------------------------------------------------------===//
    // Component
    //===------------------------------------------------------------------===//

    void longTapEvent(const MouseEvent &e) override;
    void mouseMove(const MouseEvent &e) override;
    void mouseDown(const MouseEvent &e) override;
    void mouseDrag(const MouseEvent &e) override;
    void mouseUp(const MouseEvent &e) override;
    void mouseWheelMove(const MouseEvent &e, const MouseWheelDetails &wheel) override;

    void handleCommandMessage(int commandId) override;
    void resized() override;
    void paint(Graphics &g) override;

protected:
    
    ListenerList<HybridRollListener> listeners;
    
    void broadcastRollMoved();
    void broadcastRollResized();
    
protected:
    
    //===------------------------------------------------------------------===//
    // Playhead::Listener
    //===------------------------------------------------------------------===//
    
    void onPlayheadMoved(int indicatorX) override;
    
    //===------------------------------------------------------------------===//
    // VolumeCallback::ClippingListener
    //===------------------------------------------------------------------===//
    
    void onClippingWarning() override;
    void resetAllClippingIndicators();
    OwnedArray<TimelineWarningMarker> clippingIndicators;
    
    void onOversaturationWarning() override;
    void resetAllOversaturationIndicators();
    OwnedArray<TimelineWarningMarker> oversaturationIndicators;

    //===------------------------------------------------------------------===//
    // TransportListener
    //===------------------------------------------------------------------===//
    
    void onSeek(double absolutePosition, double currentTimeMs, double totalTimeMs) override;
    void onTempoChanged(double newTempo) override;
    void onTotalTimeChanged(double timeMs) override;
    void onPlay() override;
    void onStop() override;

    Atomic<double> lastTransportPosition; // modified from a player thread

    double playheadOffset;
    bool shouldFollowPlayhead;
    
    //===------------------------------------------------------------------===//
    // AsyncUpdater
    //===------------------------------------------------------------------===//
    
    void handleAsyncUpdate() override;

    double findPlayheadOffsetFromViewCentre() const;
    friend class HybridRollHeader;
    
    //===------------------------------------------------------------------===//
    // Timer
    //===------------------------------------------------------------------===//

    void hiResTimerCallback() override;
    
protected:
    
    // These two methods are supposed to layout non-midi-event children
    virtual void updateChildrenBounds();
    virtual void updateChildrenPositions();
    virtual void setChildrenInteraction(bool interceptMouse, MouseCursor c) = 0;

    void updateBounds();
    
    WeakReference<AudioMonitor> clippingDetector;
    ProjectTreeItem &project;
    Viewport &viewport;
    
    OwnedArray<Component> trackMaps;

    Point<int> viewportAnchor;
    Point<float> clickAnchor;
    Point<float> zoomAnchor;
    ScopedPointer<Component> zoomMarker;
    
    void resetDraggingAnchors();
    void continueDragging(const MouseEvent &e);
    Point<float> getMouseOffset(Point<float> mouseScreenPosition) const;

    void startZooming();
    void continueZooming(const MouseEvent &e);
    void endZooming();
    
    
    void initWipeSpaceHelper(int xPosition);
    void updateWipeSpaceHelperIfNeeded(const MouseEvent &e);
    void removeWipeSpaceHelper();

    void startWipingSpace(const MouseEvent &e);
    void continueWipingSpace(const MouseEvent &e);
    void endWipingSpaceIfNeeded();
    
    ScopedPointer<WipeSpaceHelper> wipeSpaceHelper;

    void initInsertSpaceHelper(int xPosition);
    void updateInsertSpaceHelperIfNeeded(const MouseEvent &e);
    void removeInsertSpaceHelper();
    
    void startInsertingSpace(const MouseEvent &e);
    void continueInsertingSpace(const MouseEvent &e);
    void endInsertingSpaceIfNeeded();
    
    ScopedPointer<InsertSpaceHelper> insertSpaceHelper;

    Lasso selection;

    bool isViewportZoomEvent(const MouseEvent &e) const;
    bool isViewportDragEvent(const MouseEvent &e) const;
    bool isAddEvent(const MouseEvent &e) const;
    bool isLassoEvent(const MouseEvent &e) const;
    bool isWipeSpaceEvent(const MouseEvent &e) const;
    bool isInsertSpaceEvent(const MouseEvent &e) const;

    float firstBar;
    float lastBar;

    float projectFirstBeat;
    float projectLastBeat;

    float barWidth;
    float beatDelta;
    
    bool altDrawMode;
    bool spaceDragMode;
    int draggedDistance;
    Time timeEnteredDragMode;

    ComponentFader fader;

    ScopedPointer<HybridRollHeader> header;
    ScopedPointer<Playhead> playhead;
    
    typedef AnnotationsTrackMap<AnnotationLargeComponent> AnnotationsLargeMap;
    ScopedPointer<AnnotationsLargeMap> annotationsTrack;

    typedef TimeSignaturesTrackMap<TimeSignatureLargeComponent> TimeSignaturesLargeMap;
    ScopedPointer<TimeSignaturesLargeMap> timeSignaturesTrack;

    typedef KeySignaturesTrackMap<KeySignatureLargeComponent> KeySignaturesLargeMap;
    ScopedPointer<KeySignaturesLargeMap> keySignaturesTrack;

    ScopedPointer<Component> topShadow;
    ScopedPointer<Component> bottomShadow;

    ScopedPointer<HybridLassoComponent> lassoComponent;
    
protected:
    
    Array<float> visibleBars;
    Array<float> visibleBeats;
    Array<float> visibleSnaps;

    void computeVisibleBeatLines();

protected:

    ScopedPointer<LongTapController> longTapController;
    ScopedPointer<MultiTouchController> multiTouchController;
    ScopedPointer<SmoothPanController> smoothPanController;
    ScopedPointer<SmoothZoomController> smoothZoomController;

    Array<SafePointer<FloatBoundsComponent>> batchRepaintList;

protected:
    
    void changeListenerCallback(ChangeBroadcaster *source) override;

};
