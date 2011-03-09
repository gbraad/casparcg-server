#include "../stdafx.h"

#include "basic_frame.h"

#include "../image/image_transform.h"
#include "../image/image_mixer.h"
#include "../audio/audio_mixer.h"
#include "../audio/audio_transform.h"
#include "pixel_format.h"

#include <core/video_format.h>

#include <boost/range/algorithm.hpp>

namespace caspar { namespace core {
																																						
struct basic_frame::implementation
{		
	std::vector<safe_ptr<basic_frame>> frames_;

	image_transform image_transform_;	
	audio_transform audio_transform_;

	int index_;

public:
	implementation(const std::vector<safe_ptr<basic_frame>>& frames) 
		: frames_(frames)
		, index_(std::numeric_limits<int>::min()) {}
	implementation(std::vector<safe_ptr<basic_frame>>&& frames) 
		: frames_(std::move(frames))
		, index_(std::numeric_limits<int>::min()) {}
	
	void accept(basic_frame& self, frame_visitor& visitor)
	{
		visitor.begin(self);
		BOOST_FOREACH(auto frame, frames_)
			frame->accept(visitor);
		visitor.end();
	}	
};
	
basic_frame::basic_frame() : impl_(new implementation(std::vector<safe_ptr<basic_frame>>())){}
basic_frame::basic_frame(const std::vector<safe_ptr<basic_frame>>& frames) : impl_(new implementation(frames)){}
basic_frame::basic_frame(std::vector<safe_ptr<basic_frame>>&& frames) : impl_(new implementation(std::move(frames))){}
basic_frame::basic_frame(const basic_frame& other) : impl_(new implementation(*other.impl_)){}
basic_frame::basic_frame(const safe_ptr<basic_frame>& frame)
{
	std::vector<safe_ptr<basic_frame>> frames;
	frames.push_back(frame);
	impl_.reset(new implementation(std::move(frames)));
}
basic_frame::basic_frame(safe_ptr<basic_frame>&& frame)
{
	std::vector<safe_ptr<basic_frame>> frames;
	frames.push_back(std::move(frame));
	impl_.reset(new implementation(std::move(frames)));
}
basic_frame::basic_frame(const safe_ptr<basic_frame>& frame1, const safe_ptr<basic_frame>& frame2)
{
	std::vector<safe_ptr<basic_frame>> frames;
	frames.push_back(frame1);
	frames.push_back(frame2);
	impl_.reset(new implementation(std::move(frames)));
}
basic_frame::basic_frame(safe_ptr<basic_frame>&& frame1, safe_ptr<basic_frame>&& frame2)
{
	std::vector<safe_ptr<basic_frame>> frames;
	frames.push_back(std::move(frame1));
	frames.push_back(std::move(frame2));
	impl_.reset(new implementation(std::move(frames)));
}

void basic_frame::swap(basic_frame& other){impl_.swap(other.impl_);}
basic_frame& basic_frame::operator=(const basic_frame& other)
{
	basic_frame temp(other);
	temp.swap(*this);
	return *this;
}
basic_frame::basic_frame(basic_frame&& other) : impl_(std::move(other.impl_)){}
basic_frame& basic_frame::operator=(basic_frame&& other)
{
	basic_frame temp(std::move(other));
	temp.swap(*this);
	return *this;
}
void basic_frame::accept(frame_visitor& visitor){impl_->accept(*this, visitor);}

const image_transform& basic_frame::get_image_transform() const { return impl_->image_transform_;}
image_transform& basic_frame::get_image_transform() { return impl_->image_transform_;}
const audio_transform& basic_frame::get_audio_transform() const { return impl_->audio_transform_;}
audio_transform& basic_frame::get_audio_transform() { return impl_->audio_transform_;}

safe_ptr<basic_frame> basic_frame::interlace(const safe_ptr<basic_frame>& frame1, const safe_ptr<basic_frame>& frame2, video_mode::type mode)
{			
	if(frame1 == frame2 || mode == video_mode::progressive)
		return frame2;

	auto my_frame1 = make_safe<basic_frame>(frame1);
	auto my_frame2 = make_safe<basic_frame>(frame2);
	if(mode == video_mode::upper)
	{
		my_frame1->get_image_transform().set_mode(video_mode::upper);	
		my_frame2->get_image_transform().set_mode(video_mode::lower);	
	}											 
	else										 
	{											 
		my_frame1->get_image_transform().set_mode(video_mode::lower);	
		my_frame2->get_image_transform().set_mode(video_mode::upper);	
	}

	std::vector<safe_ptr<basic_frame>> frames;
	frames.push_back(my_frame1);
	frames.push_back(my_frame2);
	return make_safe<basic_frame>(frames);
}

void basic_frame::set_layer_index(int index) { impl_->index_ = index; }
int basic_frame::get_layer_index() const { return impl_->index_; }
std::vector<safe_ptr<basic_frame>> basic_frame::get_children(){return impl_->frames_;}
	
}}