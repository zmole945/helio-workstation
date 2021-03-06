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
#include "Clip.h"
#include "Pattern.h"
#include "MidiTrack.h"
#include "SerializationKeys.h"

Clip::Clip()
{
    // needed for juce's Array to work
    //jassertfalse;
}

Clip::Clip(const Clip &other) :
    pattern(other.pattern),
    startBeat(other.startBeat),
    id(other.id) {}

Clip::Clip(WeakReference<Pattern> owner, float beatVal) :
    pattern(owner),
    startBeat(roundBeat(beatVal))
{
    id = this->createId();
}

Clip::Clip(WeakReference<Pattern> owner, const Clip &parametersToCopy) :
    pattern(owner),
    startBeat(parametersToCopy.startBeat),
    id(parametersToCopy.id) {}

Pattern *Clip::getPattern() const noexcept
{
    jassert(this->pattern != nullptr);
    return this->pattern;
}

float Clip::getStartBeat() const noexcept
{
    return this->startBeat;
}

String Clip::getId() const noexcept
{
    return this->id;
}

bool Clip::isValid() const noexcept
{
    return this->pattern != nullptr && this->id.isNotEmpty();
}

Colour Clip::getColour() const noexcept
{
    jassert(this->pattern);
    return this->pattern->getTrack()->getTrackColour();
}

Clip Clip::copyWithNewId(Pattern *newOwner) const
{
    Clip c(*this);

    if (newOwner != nullptr)
    {
        c.pattern = newOwner;
    }

    c.id = c.createId();
    return c;
}

Clip Clip::withParameters(const XmlElement &xml) const
{
    Clip c(*this);
    c.deserialize(xml);
    return c;
}

Clip Clip::withDeltaBeat(float deltaPosition) const
{
    Clip other(*this);
    other.startBeat = roundBeat(other.startBeat + deltaPosition);
    return other;
}

XmlElement *Clip::serialize() const
{
    auto xml = new XmlElement(Serialization::Core::clip);
    xml->setAttribute("start", this->startBeat);
    xml->setAttribute("id", this->id);
    return xml;
}

void Clip::deserialize(const XmlElement &xml)
{
    this->startBeat = float(xml.getDoubleAttribute("start", this->startBeat));
    this->id = xml.getStringAttribute("id", this->id);
}

void Clip::reset()
{
    this->startBeat = 0.f;
}

int Clip::compareElements(const Clip &first, const Clip &second)
{
    if (&first == &second) { return 0; }
    if (first.id == second.id) { return 0; }

    const float diff = first.startBeat - second.startBeat;
    const int diffResult = (diff > 0.f) - (diff < 0.f);
    return diffResult;
}

int Clip::compareElements(const Clip *const first, const Clip *const second)
{
    return Clip::compareElements(*first, *second);
}

void Clip::applyChanges(const Clip &other)
{
    jassert(this->id == other.id);
    this->startBeat = other.startBeat;
}

HashCode Clip::hashCode() const noexcept
{
    const HashCode code = static_cast<HashCode>(this->startBeat)
        + static_cast<HashCode>(this->getId().hashCode());
    return code;
}

Clip::Id Clip::createId() const noexcept
{
    if (this->pattern != nullptr)
    {
        return this->pattern->createUniqueClipId();
    }

    return {};
}
