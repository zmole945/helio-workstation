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

class KeySignaturesSequence;
class MidiTrackSource;

#include "KeySignatureEvent.h"
#include "UndoAction.h"


//===----------------------------------------------------------------------===//
// Insert
//===----------------------------------------------------------------------===//

class KeySignatureEventInsertAction : public UndoAction
{
public:
    
    explicit KeySignatureEventInsertAction(MidiTrackSource &source) :
    UndoAction(source) {}

    KeySignatureEventInsertAction(MidiTrackSource &source,
                                   String trackId,
                                   const KeySignatureEvent &target);

    bool perform() override;
    bool undo() override;
    int getSizeInUnits() override;
    
    XmlElement *serialize() const override;
    void deserialize(const XmlElement &xml) override;
    void reset() override;

private:

    String trackId;
    KeySignatureEvent event;

    JUCE_DECLARE_NON_COPYABLE(KeySignatureEventInsertAction)
};


//===----------------------------------------------------------------------===//
// Remove
//===----------------------------------------------------------------------===//

class KeySignatureEventRemoveAction : public UndoAction
{
public:
    
    explicit KeySignatureEventRemoveAction(MidiTrackSource &source) :
    UndoAction(source) {}

    KeySignatureEventRemoveAction(MidiTrackSource &source,
                                   String trackId,
                                   const KeySignatureEvent &target);

    bool perform() override;
    bool undo() override;
    int getSizeInUnits() override;
    
    XmlElement *serialize() const override;
    void deserialize(const XmlElement &xml) override;
    void reset() override;

private:

    String trackId;
    KeySignatureEvent event;

    JUCE_DECLARE_NON_COPYABLE(KeySignatureEventRemoveAction)
};


//===----------------------------------------------------------------------===//
// Change
//===----------------------------------------------------------------------===//

class KeySignatureEventChangeAction : public UndoAction
{
public:
    
    explicit KeySignatureEventChangeAction(MidiTrackSource &source) :
    UndoAction(source) {}

    KeySignatureEventChangeAction(MidiTrackSource &source,
                                   String trackId,
                                   const KeySignatureEvent &target,
                                   const KeySignatureEvent &newParameters);

    bool perform() override;
    bool undo() override;
    int getSizeInUnits() override;
    UndoAction *createCoalescedAction(UndoAction *nextAction) override;
    
    XmlElement *serialize() const override;
    void deserialize(const XmlElement &xml) override;
    void reset() override;

private:

    String trackId;
    
    KeySignatureEvent eventBefore;
    KeySignatureEvent eventAfter;

    JUCE_DECLARE_NON_COPYABLE(KeySignatureEventChangeAction)

};


//===----------------------------------------------------------------------===//
// Insert Group
//===----------------------------------------------------------------------===//

class KeySignatureEventsGroupInsertAction : public UndoAction
{
public:
    
    explicit KeySignatureEventsGroupInsertAction(MidiTrackSource &source) :
    UndoAction(source) {}
    
    KeySignatureEventsGroupInsertAction(MidiTrackSource &source,
                                         String trackId,
                                         Array<KeySignatureEvent> &target);
    
    bool perform() override;
    bool undo() override;
    int getSizeInUnits() override;
    
    XmlElement *serialize() const override;
    void deserialize(const XmlElement &xml) override;
    void reset() override;
    
private:
    
    String trackId;
    Array<KeySignatureEvent> signatures;
    
    JUCE_DECLARE_NON_COPYABLE(KeySignatureEventsGroupInsertAction)
    
};


//===----------------------------------------------------------------------===//
// Remove Group
//===----------------------------------------------------------------------===//

class KeySignatureEventsGroupRemoveAction : public UndoAction
{
public:
    
    explicit KeySignatureEventsGroupRemoveAction(MidiTrackSource &source) :
    UndoAction(source) {}
    
    KeySignatureEventsGroupRemoveAction(MidiTrackSource &source,
                                         String trackId,
                                         Array<KeySignatureEvent> &target);
    
    bool perform() override;
    bool undo() override;
    int getSizeInUnits() override;
    
    XmlElement *serialize() const override;
    void deserialize(const XmlElement &xml) override;
    void reset() override;
    
private:
    
    String trackId;
    Array<KeySignatureEvent> signatures;
    
    JUCE_DECLARE_NON_COPYABLE(KeySignatureEventsGroupRemoveAction)
    
};


//===----------------------------------------------------------------------===//
// Change Group
//===----------------------------------------------------------------------===//

class KeySignatureEventsGroupChangeAction : public UndoAction
{
public:
    
    explicit KeySignatureEventsGroupChangeAction(MidiTrackSource &source) :
    UndoAction(source) {}

    KeySignatureEventsGroupChangeAction(MidiTrackSource &source,
                                         String trackId,
                                         const Array<KeySignatureEvent> state1,
                                         const Array<KeySignatureEvent> state2);

    bool perform() override;
    bool undo() override;
    int getSizeInUnits() override;
    UndoAction *createCoalescedAction(UndoAction *nextAction) override;
    
    XmlElement *serialize() const override;
    void deserialize(const XmlElement &xml) override;
    void reset() override;

private:

    String trackId;
    
    Array<KeySignatureEvent> eventsBefore;
    Array<KeySignatureEvent> eventsAfter;

    JUCE_DECLARE_NON_COPYABLE(KeySignatureEventsGroupChangeAction)

};

