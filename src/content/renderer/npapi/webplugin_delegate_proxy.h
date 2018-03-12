// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_NPAPI_WEBPLUGIN_DELEGATE_PROXY_H_
#define CONTENT_RENDERER_NPAPI_WEBPLUGIN_DELEGATE_PROXY_H_

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/sequenced_task_runner_helpers.h"
#include "build/build_config.h"
#include "content/child/npapi/webplugin_delegate.h"
#include "content/public/common/webplugininfo.h"
#include "ipc/ipc_listener.h"
#include "ipc/ipc_message.h"
#include "ipc/ipc_sender.h"
#include "third_party/skia/include/core/SkRefCnt.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/native_widget_types.h"
#include "url/gurl.h"

#if defined(OS_MACOSX)
#include "base/containers/hash_tables.h"
#endif

struct NPObject;
struct PluginHostMsg_URLRequest_Params;
class SkBitmap;

namespace base {
class WaitableEvent;
}

#if defined(OS_WEBOS)
namespace gfx {
struct GpuMemoryBufferHandle;
}
#endif

namespace content {
class NPObjectStub;
class PluginChannelHost;
class RenderFrameImpl;
class RenderViewImpl;
class SharedMemoryBitmap;
class WebPluginImpl;

// An implementation of WebPluginDelegate that proxies all calls to
// the plugin process.
class WebPluginDelegateProxy
    : public WebPluginDelegate,
      public IPC::Listener,
      public IPC::Sender,
      public base::SupportsWeakPtr<WebPluginDelegateProxy> {
 public:
  WebPluginDelegateProxy(WebPluginImpl* plugin,
                         const std::string& mime_type,
                         const base::WeakPtr<RenderViewImpl>& render_view,
                         RenderFrameImpl* render_frame);

  // WebPluginDelegate implementation:
  void PluginDestroyed() override;
  bool Initialize(const GURL& url,
                  const std::vector<std::string>& arg_names,
                  const std::vector<std::string>& arg_values,
                  bool load_manually) override;
  void UpdateGeometry(const gfx::Rect& window_rect,
                      const gfx::Rect& clip_rect) override;
  void Paint(SkCanvas* canvas, const gfx::Rect& rect) override;
  NPObject* GetPluginScriptableObject() override;
  struct _NPP* GetPluginNPP() override;
  bool GetFormValue(base::string16* value) override;
  void DidFinishLoadWithReason(const GURL& url,
                               NPReason reason,
                               int notify_id) override;
  void SetFocus(bool focused) override;
  bool HandleInputEvent(const blink::WebInputEvent& event,
                        WebCursor::CursorInfo* cursor) override;
  int GetProcessId() override;

#if defined(OS_WEBOS)
  void Suspend() override;
  void Resume() override;
  void SetDeviceInfo(const gfx::Size& device_viewport_size,
                     const gfx::Size& view_size) override;
  void AcceleratedPluginDidSwapBuffer(int32_t id);
  void AcceleratedPluginDidSwapBufferComplete(int32_t id);
  void OnSetUsesPunchHoleForVideo(bool uses_punch_hole_for_video);
#endif

  // Informs the plugin that its containing content view has gained or lost
  // first responder status.
  virtual void SetContentAreaFocus(bool has_focus);
#if defined(OS_MACOSX)
  // Informs the plugin that its enclosing window has gained or lost focus.
  virtual void SetWindowFocus(bool window_has_focus);
  // Informs the plugin that its container (window/tab) has changed visibility.
  virtual void SetContainerVisibility(bool is_visible);
  // Informs the plugin that its enclosing window's frame has changed.
  virtual void WindowFrameChanged(gfx::Rect window_frame, gfx::Rect view_frame);
  // Informs the plugin that plugin IME has completed.
  // If |text| is empty, composition was cancelled.
  virtual void ImeCompositionCompleted(const base::string16& text,
                                       int plugin_id);
#endif

  // IPC::Listener implementation:
  bool OnMessageReceived(const IPC::Message& msg) override;
  void OnChannelError() override;

  // IPC::Sender implementation:
  bool Send(IPC::Message* msg) override;

  void SendJavaScriptStream(const GURL& url,
                            const std::string& result,
                            bool success,
                            int notify_id) override;

  void DidReceiveManualResponse(const GURL& url,
                                const std::string& mime_type,
                                const std::string& headers,
                                uint32_t expected_length,
                                uint32_t last_modified) override;
  void DidReceiveManualData(const char* buffer, int length) override;
  void DidFinishManualLoading() override;
  void DidManualLoadFail() override;
  WebPluginResourceClient* CreateResourceClient(unsigned long resource_id,
                                                const GURL& url,
                                                int notify_id) override;
  WebPluginResourceClient* CreateSeekableResourceClient(
      unsigned long resource_id,
      int range_request_id) override;
  void FetchURL(unsigned long resource_id,
                int notify_id,
                const GURL& url,
                const GURL& first_party_for_cookies,
                const std::string& method,
                const char* buf,
                unsigned int len,
                const Referrer& referrer,
                bool notify_redirects,
                bool is_plugin_src_load,
                int origin_pid,
                int render_frame_id,
                int render_view_id) override;

 protected:
  friend class base::DeleteHelper<WebPluginDelegateProxy>;
  ~WebPluginDelegateProxy() override;

 private:
  struct SharedBitmap {
    SharedBitmap();
    ~SharedBitmap();

    std::unique_ptr<SharedMemoryBitmap> bitmap;
    std::unique_ptr<SkCanvas> canvas;
  };

  // Message handlers for messages that proxy WebPlugin methods, which
  // we translate into calls to the real WebPlugin.
  void OnCompleteURL(const std::string& url_in, std::string* url_out,
                     bool* result);
  void OnHandleURLRequest(const PluginHostMsg_URLRequest_Params& params);
  void OnCancelResource(int id);
  void OnInvalidateRect(const gfx::Rect& rect);
  void OnGetWindowScriptNPObject(int route_id, bool* success);
  void OnResolveProxy(const GURL& url, bool* result, std::string* proxy_list);
  void OnGetPluginElement(int route_id, bool* success);
  void OnSetCookie(const GURL& url,
                   const GURL& first_party_for_cookies,
                   const std::string& cookie);
  void OnGetCookies(const GURL& url, const GURL& first_party_for_cookies,
                    std::string* cookies);
  void OnCancelDocumentLoad();
  void OnInitiateHTTPRangeRequest(const std::string& url,
                                  const std::string& range_info,
                                  int range_request_id);
  void OnDidStartLoading();
  void OnDidStopLoading();
  void OnDeferResourceLoading(unsigned long resource_id, bool defer);
  void OnURLRedirectResponse(bool allow, int resource_id);
  void OnGetUserAgent(std::string *user_agent);
#if defined(OS_MACOSX)
  void OnFocusChanged(bool focused);
  void OnStartIme();
  // Accelerated (Core Animation) plugin implementation.
  void OnAcceleratedPluginEnabledRendering();
  void OnAcceleratedPluginAllocatedIOSurface(int32_t width,
                                             int32_t height,
                                             uint32_t surface_id);
  void OnAcceleratedPluginSwappedIOSurface();
#endif
#if defined(OS_WEBOS)
  void OnPluginEnabledRendering();
  // Accelerated plugin implementation.
  void OnAcceleratedPluginEnabledRendering(bool paintless);
  void OnAcceleratedPluginAllocateBuffer(uint32_t width,
                                         uint32_t height,
                                         uint32_t internalformat,
                                         uint32_t usage,
                                         uint32_t format,
                                         int32_t* id,
                                         gfx::GpuMemoryBufferHandle* handle);
  void OnAcceleratedPluginReleaseBuffer(int32_t id);
  void OnAcceleratedPluginSwapBuffer(int32_t id);
#endif

  // Helper function that sends the UpdateGeometry message.
  void SendUpdateGeometry(bool bitmaps_changed);

  // Copies the given rectangle from the back-buffer transport_stores_ bitmap to
  // the front-buffer transport_stores_ bitmap.
  void CopyFromBackBufferToFrontBuffer(const gfx::Rect& rect);

  // Updates the front-buffer with the given rectangle from the back-buffer,
  // either by copying the rectangle or flipping the buffers.
  void UpdateFrontBuffer(const gfx::Rect& rect, bool allow_buffer_flipping);

  // Clears the shared memory section and canvases used for windowless plugins.
  void ResetWindowlessBitmaps();

  int front_buffer_index() const {
    return front_buffer_index_;
  }

  int back_buffer_index() const {
    return 1 - front_buffer_index_;
  }

  SkCanvas* front_buffer_canvas() const {
    return transport_stores_[front_buffer_index()].canvas.get();
  }

  SkCanvas* back_buffer_canvas() const {
    return transport_stores_[back_buffer_index()].canvas.get();
  }

  SharedMemoryBitmap* front_buffer_bitmap() const {
    return transport_stores_[front_buffer_index()].bitmap.get();
  }

  SharedMemoryBitmap* back_buffer_bitmap() const {
    return transport_stores_[back_buffer_index()].bitmap.get();
  }

#if !defined(OS_WIN)
  // Creates a process-local memory section and canvas. PlatformCanvas on
  // Windows only works with a DIB, not arbitrary memory.
  bool CreateLocalBitmap(std::vector<uint8_t>* memory,
                         std::unique_ptr<SkCanvas>* canvas);
#endif

  // Creates a shared memory section and canvas.
  bool CreateSharedBitmap(std::unique_ptr<SharedMemoryBitmap>* memory,
                          std::unique_ptr<SkCanvas>* canvas);

  // Called for cleanup during plugin destruction. Normally right before the
  // plugin window gets destroyed, or when the plugin has crashed (at which
  // point the window has already been destroyed).
  void WillDestroyWindow();

#if defined(OS_WIN)
  // Returns true if we should update the plugin geometry synchronously.
  bool UseSynchronousGeometryUpdates();
#endif

  base::WeakPtr<RenderViewImpl> render_view_;
  RenderFrameImpl* render_frame_;
  WebPluginImpl* plugin_;
  bool uses_shared_bitmaps_;
#if defined(OS_MACOSX) || defined(OS_WEBOS)
  bool uses_compositor_;
#endif
  scoped_refptr<PluginChannelHost> channel_host_;
  std::string mime_type_;
  int instance_id_;
  WebPluginInfo info_;

  gfx::Rect plugin_rect_;
  gfx::Rect clip_rect_;

  NPObject* npobject_;

  // Dummy NPP used to uniquely identify this plugin.
  std::unique_ptr<NPP_t> npp_;

  // Bitmap for crashed plugin
  SkBitmap* sad_plugin_;

  // True if we got an invalidate from the plugin and are waiting for a paint.
  bool invalidate_pending_;

  // If the plugin is transparent or not.
  bool transparent_;

  // The index in the transport_stores_ array of the current front buffer
  // (i.e., the buffer to display).
  int front_buffer_index_;
  SharedBitmap transport_stores_[2];
  // This lets us know the total portion of the transport store that has been
  // painted since the buffers were created.
  gfx::Rect transport_store_painted_;
  // This is a bounding box on the portion of the front-buffer that was painted
  // on the last buffer flip and which has not yet been re-painted in the
  // back-buffer.
  gfx::Rect front_buffer_diff_;

  // The url of the main frame hosting the plugin.
  GURL page_url_;

  DISALLOW_COPY_AND_ASSIGN(WebPluginDelegateProxy);
};

}  // namespace content

#endif  // CONTENT_RENDERER_NPAPI_WEBPLUGIN_DELEGATE_PROXY_H_