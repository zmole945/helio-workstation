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

#include "Common.h"
#include "TimeSignaturesSequence.h"
#include "Note.h"
#include "TimeSignatureEventActions.h"
#include "SerializationKeys.h"
#include "ProjectTreeItem.h"
#include "UndoStack.h"

TimeSignaturesSequence::TimeSignaturesSequence(MidiTrack &track,
    ProjectEventDispatcher &dispatcher) :
    MidiSequence(track, dispatcher) {}

//===----------------------------------------------------------------------===//
// Import/export
//===----------------------------------------------------------------------===//

void TimeSignaturesSequence::importMidi(const MidiMessageSequence &sequence)
{
    this->clearUndoHistory();
    this->checkpoint();
    this->reset();

    for (int i = 0; i < sequence.getNumEvents(); ++i)
    {
        const MidiMessage &message = sequence.getEventPointer(i)->message;

        if (message.isTimeSignatureMetaEvent())
        {
            int numerator = 0;
            int denominator = 0;
            message.getTimeSignatureInfo(numerator, denominator);
            const double startTimestamp = message.getTimeStamp() / MIDI_IMPORT_SCALE;
            const float beat = float(startTimestamp);
            const TimeSignatureEvent signature(this, beat, numerator, denominator);
            this->silentImport(signature);
        }
    }

    this->updateBeatRange(false);
    this->invalidateSequenceCache();
}

//===----------------------------------------------------------------------===//
// Undoable track editing
//===----------------------------------------------------------------------===//

void TimeSignaturesSequence::silentImport(const MidiEvent &eventToImport)
{
    const TimeSignatureEvent &signature =
        static_cast<const TimeSignatureEvent &>(eventToImport);

    if (this->usedEventIds.contains(signature.getId()))
    {
        jassertfalse;
        return;
    }

    TimeSignatureEvent *storedSignature = new TimeSignatureEvent(this, signature);
    
    this->midiEvents.addSorted(*storedSignature, storedSignature);
    this->usedEventIds.insert(storedSignature->getId());

    this->updateBeatRange(false);
    this->invalidateSequenceCache();
}

MidiEvent *TimeSignaturesSequence::insert(const TimeSignatureEvent &eventParams, bool undoable)
{
    if (undoable)
    {
        this->getUndoStack()->
            perform(new TimeSignatureEventInsertAction(*this->getProject(),
                this->getTrackId(), eventParams));
    }
    else
    {
        const auto ownedEvent = new TimeSignatureEvent(this, eventParams);
        this->midiEvents.addSorted(*ownedEvent, ownedEvent);
        this->notifyEventAdded(*ownedEvent);
        this->updateBeatRange(true);
        return ownedEvent;
    }

    return nullptr;
}

bool TimeSignaturesSequence::remove(const TimeSignatureEvent &signature, bool undoable)
{
    if (undoable)
    {
        this->getUndoStack()->
            perform(new TimeSignatureEventRemoveAction(*this->getProject(),
                this->getTrackId(), signature));
    }
    else
    {
        const int index = this->midiEvents.indexOfSorted(signature, &signature);
        if (index >= 0)
        {
            MidiEvent *const removedEvent = this->midiEvents[index];
            this->notifyEventRemoved(*removedEvent);
            this->midiEvents.remove(index, true);
            this->updateBeatRange(true);
            this->notifyEventRemovedPostAction();
            return true;
        }

        return false;
    }

    return true;
}

bool TimeSignaturesSequence::change(const TimeSignatureEvent &oldParams,
    const TimeSignatureEvent &newParams, bool undoable)
{
    if (undoable)
    {
        this->getUndoStack()->
            perform(new TimeSignatureEventChangeAction(*this->getProject(),
                this->getTrackId(), oldParams, newParams));
    }
    else
    {
        const int index = this->midiEvents.indexOfSorted(oldParams, &oldParams);
        if (index >= 0)
        {
            auto changedEvent = static_cast<TimeSignatureEvent *>(this->midiEvents[index]);
            changedEvent->applyChanges(newParams);
            this->midiEvents.remove(index, false);
            this->midiEvents.addSorted(*changedEvent, changedEvent);
            this->notifyEventChanged(oldParams, *changedEvent);
            this->updateBeatRange(true);
            return true;
        }
        
        return false;
    }

    return true;
}

bool TimeSignaturesSequence::insertGroup(Array<TimeSignatureEvent> &signatures, bool undoable)
{
    if (undoable)
    {
        this->getUndoStack()->
            perform(new TimeSignatureEventsGroupInsertAction(*this->getProject(),
                this->getTrackId(), signatures));
    }
    else
    {
        for (int i = 0; i < signatures.size(); ++i)
        {
            const TimeSignatureEvent &eventParams = signatures.getUnchecked(i);
            const auto ownedEvent = new TimeSignatureEvent(this, eventParams);
            this->midiEvents.addSorted(*ownedEvent, ownedEvent);
            this->notifyEventAdded(*ownedEvent);
        }
        
        this->updateBeatRange(true);
    }
    
    return true;
}

bool TimeSignaturesSequence::removeGroup(Array<TimeSignatureEvent> &signatures, bool undoable)
{
    if (undoable)
    {
        this->getUndoStack()->
            perform(new TimeSignatureEventsGroupRemoveAction(*this->getProject(),
                this->getTrackId(), signatures));
    }
    else
    {
        for (int i = 0; i < signatures.size(); ++i)
        {
            const TimeSignatureEvent &signature = signatures.getUnchecked(i);
            const int index = this->midiEvents.indexOfSorted(signature, &signature);
            if (index >= 0)
            {
                MidiEvent *const removedSignature = this->midiEvents[index];
                this->notifyEventRemoved(*removedSignature);
                this->midiEvents.remove(index, true);
            }
        }
        
        this->updateBeatRange(true);
        this->notifyEventRemovedPostAction();
    }
    
    return true;
}

bool TimeSignaturesSequence::changeGroup(Array<TimeSignatureEvent> &groupBefore,
    Array<TimeSignatureEvent> &groupAfter, bool undoable)
{
    jassert(groupBefore.size() == groupAfter.size());

    if (undoable)
    {
        this->getUndoStack()->
            perform(new TimeSignatureEventsGroupChangeAction(*this->getProject(),
                this->getTrackId(), groupBefore, groupAfter));
    }
    else
    {
        for (int i = 0; i < groupBefore.size(); ++i)
        {
            const TimeSignatureEvent &oldParams = groupBefore.getUnchecked(i);
            const TimeSignatureEvent &newParams = groupAfter.getUnchecked(i);
            const int index = this->midiEvents.indexOfSorted(oldParams, &oldParams);
            if (index >= 0)
            {
                const auto changedEvent = static_cast<TimeSignatureEvent *>(this->midiEvents[index]);
                changedEvent->applyChanges(newParams);
                this->midiEvents.remove(index, false);
                this->midiEvents.addSorted(*changedEvent, changedEvent);
                this->notifyEventChanged(oldParams, *changedEvent);
            }
        }

        this->updateBeatRange(true);
    }

    return true;
}


//===----------------------------------------------------------------------===//
// Serializable
//===----------------------------------------------------------------------===//

XmlElement *TimeSignaturesSequence::serialize() const
{
    auto xml = new XmlElement(Serialization::Core::timeSignatures);

    for (int i = 0; i < this->midiEvents.size(); ++i)
    {
        const MidiEvent *event = this->midiEvents.getUnchecked(i);
        xml->prependChildElement(event->serialize()); // faster than addChildElement
    }

    return xml;
}

void TimeSignaturesSequence::deserialize(const XmlElement &xml)
{
    this->reset();

    const XmlElement *root =
        (xml.getTagName() == Serialization::Core::timeSignatures) ?
        &xml : xml.getChildByName(Serialization::Core::timeSignatures);

    if (root == nullptr)
    {
        return;
    }

    float lastBeat = 0;
    float firstBeat = 0;

    forEachXmlChildElementWithTagName(*root, e, Serialization::Core::timeSignature)
    {
        TimeSignatureEvent *signature = new TimeSignatureEvent(this);
        signature->deserialize(*e);
        
        this->midiEvents.add(signature); // sorted later

        lastBeat = jmax(lastBeat, signature->getBeat());
        firstBeat = jmin(firstBeat, signature->getBeat());

        this->usedEventIds.insert(signature->getId());
    }

    this->sort();
    this->updateBeatRange(false);
    this->invalidateSequenceCache();
}

void TimeSignaturesSequence::reset()
{
    this->midiEvents.clear();
    this->usedEventIds.clear();
    this->invalidateSequenceCache();
}
