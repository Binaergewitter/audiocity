/**********************************************************************

  Audacity: A Digital Audio Editor

  @file ChannelAttachments.cpp

  Dominic Mazzoni

  Paul Licameli split from Track.cpp

*//*******************************************************************/
#include "ChannelAttachments.h"

ChannelAttachment::~ChannelAttachment() = default;

ChannelAttachmentsBase &
ChannelAttachmentsBase::operator=(ChannelAttachmentsBase &&other)
{
   assert(typeid(*this) == typeid(other));
   mAttachments = move(other.mAttachments);
   return *this;
}

void ChannelAttachment::CopyTo(Track&, size_t) const
{
}

void ChannelAttachment::Reparent(const std::shared_ptr<Track> &, size_t)
{
}

void ChannelAttachment::WriteXMLAttributes(XMLWriter &, size_t) const
{
}

bool ChannelAttachment::HandleXMLAttribute(
   const std::string_view &, const XMLAttributeValueView &, size_t)
{
   return false;
}

ChannelAttachment &ChannelAttachmentsBase::Get(
   const AttachedTrackObjects::RegisteredFactory &key,
   Track &track, size_t iChannel)
{
   // Precondition of this function; satisfies precondition of factory below
   assert(iChannel < track.NChannels());
   auto &attachments = track.AttachedObjects::Get<ChannelAttachmentsBase>(key);
   auto &objects = attachments.mAttachments;
   if (iChannel >= objects.size())
      objects.resize(iChannel + 1);
   auto &pObject = objects[iChannel];
   if (!pObject) {
      // Create on demand
      pObject = attachments.mFactory(track, iChannel);
      assert(pObject); // Precondition of constructor
   }
   return *pObject;
}

ChannelAttachment *ChannelAttachmentsBase::Find(
   const AttachedTrackObjects::RegisteredFactory &key,
   Track *pTrack, size_t iChannel)
{
   assert(!pTrack || iChannel < pTrack->NChannels());
   if (!pTrack)
      return nullptr;
   const auto pAttachments =
      pTrack->AttachedObjects::Find<ChannelAttachmentsBase>(key);
   // do not create on demand
   if (!pAttachments || iChannel >= pAttachments->mAttachments.size())
      return nullptr;
   return pAttachments->mAttachments[iChannel].get();
}

ChannelAttachmentsBase::ChannelAttachmentsBase(Track &track, Factory factory)
   : mFactory{ move(factory) }
{
   // Always construct one channel view
   // TODO wide wave tracks -- number of channels will be known earlier, and
   // they will all be constructed
   mAttachments.push_back(mFactory(track, 0));
}

ChannelAttachmentsBase::~ChannelAttachmentsBase() = default;

void ChannelAttachmentsBase::CopyTo(Track &track) const
{
   // Maybe making a narrow empty copy so limit to the other track's number
   // of channels
   const size_t nn = std::min(mAttachments.size(), track.NChannels());
   for (size_t ii = 0; ii < nn; ++ii) {
      if (mAttachments[ii])
         mAttachments[ii]->CopyTo(track, ii);
   }
}

void ChannelAttachmentsBase::Reparent(const std::shared_ptr<Track> &parent)
{
   const size_t nn = mAttachments.size();
   for (size_t ii = 0; ii < nn; ++ii) {
      if (mAttachments[ii])
         mAttachments[ii]->Reparent(parent, ii);
   }
}

void ChannelAttachmentsBase::WriteXMLAttributes(XMLWriter &writer) const
{
   const size_t nn = mAttachments.size();
   for (size_t ii = 0; ii < nn; ++ii) {
      if (mAttachments[ii])
         mAttachments[ii]->WriteXMLAttributes(writer, ii);
   }
}

bool ChannelAttachmentsBase::HandleXMLAttribute(
   const std::string_view& attr, const XMLAttributeValueView& valueView)
{
   size_t ii = 0;
   return any_of(mAttachments.begin(), mAttachments.end(),
   [&](auto &pAttachment) {
      bool result = pAttachment &&
         pAttachment->HandleXMLAttribute(attr, valueView, ii);
      ++ii;
      return result;
   });
}

void ChannelAttachmentsBase::MakeStereo(ChannelAttachmentsBase &&other)
{
   assert(typeid(*this) == typeid(other));
   assert(Size() <= 1);
   assert(other.Size() <= 1);
   if (mAttachments.empty())
      mAttachments.resize(1);
   for (auto &ptr : other.mAttachments)
      mAttachments.emplace_back(move(ptr));
   other.mAttachments.clear();
}

void ChannelAttachmentsBase::SwapChannels()
{
   assert(Size() <= 2);
   if (mAttachments.empty())
      return;
   mAttachments.resize(2);
   std::swap(mAttachments[0], mAttachments[1]);
}
