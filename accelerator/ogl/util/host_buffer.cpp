/*
* Copyright (c) 2011 Sveriges Television AB <info@casparcg.com>
*
* This file is part of CasparCG (www.casparcg.com).
*
* CasparCG is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* CasparCG is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with CasparCG. If not, see <http://www.gnu.org/licenses/>.
*
* Author: Robert Nagy, ronag89@gmail.com
*/

#include "../../stdafx.h"

#include "host_buffer.h"

#include "device_buffer.h"
#include "context.h"

#include <common/except.h>
#include <common/gl/gl_check.h>

#include <gl/glew.h>

#include <tbb/atomic.h>

namespace caspar { namespace accelerator { namespace ogl {

static tbb::atomic<int> g_w_total_count;
static tbb::atomic<int> g_r_total_count;
																																								
struct host_buffer::impl : boost::noncopyable
{	
	GLuint						pbo_;
	const int					size_;
	tbb::atomic<void*>			data_;
	GLenum						usage_;
	GLenum						target_;

public:
	impl(int size, host_buffer::usage usage) 
		: size_(size)
		, pbo_(0)
		, target_(usage == host_buffer::usage::write_only ? GL_PIXEL_UNPACK_BUFFER : GL_PIXEL_PACK_BUFFER)
		, usage_(usage == host_buffer::usage::write_only ? GL_STREAM_DRAW : GL_STREAM_READ)
	{
		data_ = nullptr;
		GL(glGenBuffers(1, &pbo_));
		bind();
		if(usage_ != GL_STREAM_DRAW)	
			GL(glBufferData(target_, size_, NULL, usage_));	
		unbind();

		if(!pbo_)
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("Failed to allocate buffer."));

		CASPAR_LOG(trace) << "[host_buffer] [" << ++(usage_ == host_buffer::usage::write_only ? g_w_total_count : g_r_total_count) << L"] allocated size:" << size_ << " usage: " << (usage == host_buffer::usage::write_only ? "write_only" : "read_only");
	}	

	~impl()
	{
		glDeleteBuffers(1, &pbo_);
	}

	void* map()
	{
		if(data_ != nullptr)
			return data_;

		GL(glBindBuffer(target_, pbo_));
		if(usage_ == GL_STREAM_DRAW)			
			GL(glBufferData(target_, size_, NULL, usage_));	// Notify OpenGL that we don't care about previous data.
		
		data_ = GL2(glMapBuffer(target_, usage_ == GL_STREAM_DRAW ? GL_WRITE_ONLY : GL_READ_ONLY));  
		GL(glBindBuffer(target_, 0));
		if(!data_)
			BOOST_THROW_EXCEPTION(invalid_operation() << msg_info("Failed to map target OpenGL Pixel Buffer Object."));

		return data_;
	}
	
	void unmap()
	{
		if(data_ == nullptr)
			return;
		
		GL(glBindBuffer(target_, pbo_));
		GL(glUnmapBuffer(target_));	
		if(usage_ == GL_STREAM_READ)			
			GL(glBufferData(target_, size_, NULL, usage_));	// Notify OpenGL that we don't care about previous data.
		data_ = nullptr;	
		GL(glBindBuffer(target_, 0));
	}

	void bind()
	{
		GL(glBindBuffer(target_, pbo_));
	}

	void unbind()
	{
		GL(glBindBuffer(target_, 0));
	}
};

host_buffer::host_buffer(int size, usage usage) : impl_(new impl(size, usage)){}
void* host_buffer::data(){return impl_->data_;}
void host_buffer::map(){impl_->map();}
void host_buffer::unmap(){impl_->unmap();}
void host_buffer::bind() const{impl_->bind();}
void host_buffer::unbind() const{impl_->unbind();}
int host_buffer::size() const { return impl_->size_; }

}}}