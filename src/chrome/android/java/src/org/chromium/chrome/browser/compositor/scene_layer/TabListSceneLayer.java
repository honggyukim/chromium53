// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.compositor.scene_layer;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.Rect;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.compositor.LayerTitleCache;
import org.chromium.chrome.browser.compositor.layouts.Layout;
import org.chromium.chrome.browser.compositor.layouts.Layout.Orientation;
import org.chromium.chrome.browser.compositor.layouts.components.LayoutTab;
import org.chromium.chrome.browser.compositor.layouts.content.TabContentManager;
import org.chromium.chrome.browser.util.ColorUtils;
import org.chromium.chrome.browser.util.FeatureUtilities;
import org.chromium.ui.resources.ResourceManager;

/**
 * A SceneLayer to render a tab stack.
 * TODO(changwan): change layouts to share one instance of this.
 */
@JNINamespace("chrome::android")
public class TabListSceneLayer extends SceneLayer {
    private long mNativePtr;

    /**
     * Pushes all relevant {@link LayoutTab}s from a {@link Layout} to the CC Layer tree.  This will
     * let them be rendered on the screen.  This should only be called when the Compositor has
     * disabled ScheduleComposite calls as this will change the tree and could subsequently cause
     * unnecessary follow up renders.
     * @param context         The {@link Context} to use to query device information.
     * @param layout          The {@link Layout} to push to the screen.
     */
    public void pushLayers(Context context, Rect viewport, Rect contentViewport, Layout layout,
            LayerTitleCache layerTitleCache, TabContentManager tabContentManager,
            ResourceManager resourceManager) {
        if (mNativePtr == 0) return;

        Resources res = context.getResources();
        final float dpToPx = res.getDisplayMetrics().density;

        LayoutTab[] tabs = layout.getLayoutTabsToRender();
        int tabsCount = tabs != null ? tabs.length : 0;

        nativeBeginBuildingFrame(mNativePtr);

        nativeUpdateLayer(mNativePtr, getTabListBackgroundColor(context), viewport.left,
                viewport.top, viewport.width(), viewport.height(), layerTitleCache,
                tabContentManager, resourceManager);

        for (int i = 0; i < tabsCount; i++) {
            LayoutTab t = tabs[i];
            assert t.isVisible() : "LayoutTab in that list should be visible";
            final float decoration = t.getDecorationAlpha();

            int defaultThemeColor = t.isIncognito()
                    ? ApiCompatibilityUtils.getColor(res, R.color.incognito_primary_color)
                    : ApiCompatibilityUtils.getColor(res, R.color.default_primary_color);

            // If theme colors are enabled in the tab switcher, the theme might require lighter
            // text.
            boolean isDarkTheme = t.isIncognito();
            if (FeatureUtilities.areTabSwitcherThemeColorsEnabled()) {
                isDarkTheme |= ColorUtils.shouldUseLightForegroundOnBackground(
                        t.getToolbarBackgroundColor());
            }

            int closeBtnResource = isDarkTheme ? R.drawable.btn_tab_close_white_normal
                                               : R.drawable.btn_tab_close_normal;
            int borderColorResource =
                    t.isIncognito() ? R.color.tab_back_incognito : R.color.tab_back;
            // TODO(dtrainor, clholgat): remove "* dpToPx" once the native part fully supports dp.
            nativePutTabLayer(mNativePtr, t.getId(), R.id.control_container, closeBtnResource,
                    R.drawable.tabswitcher_border_frame_shadow,
                    R.drawable.tabswitcher_border_frame_decoration, R.drawable.logo_card_back,
                    R.drawable.tabswitcher_border_frame,
                    R.drawable.tabswitcher_border_frame_inner_shadow,
                    t.canUseLiveTexture(), t.getBackgroundColor(),
                    ApiCompatibilityUtils.getColor(res, borderColorResource), t.isIncognito(),
                    layout.getOrientation() == Orientation.PORTRAIT, t.getRenderX() * dpToPx,
                    t.getRenderY() * dpToPx, t.getScaledContentWidth() * dpToPx,
                    t.getScaledContentHeight() * dpToPx, t.getOriginalContentWidth() * dpToPx,
                    t.getOriginalContentHeight() * dpToPx, contentViewport.height(),
                    t.getClippedX() * dpToPx, t.getClippedY() * dpToPx,
                    Math.min(t.getClippedWidth(), t.getScaledContentWidth()) * dpToPx,
                    Math.min(t.getClippedHeight(), t.getScaledContentHeight()) * dpToPx,
                    t.getTiltXPivotOffset() * dpToPx, t.getTiltYPivotOffset() * dpToPx,
                    t.getTiltX(), t.getTiltY(), t.getAlpha(), t.getBorderAlpha() * decoration,
                    t.getBorderInnerShadowAlpha() * decoration, decoration,
                    t.getShadowOpacity() * decoration, t.getBorderCloseButtonAlpha() * decoration,
                    LayoutTab.CLOSE_BUTTON_WIDTH_DP * dpToPx, t.getStaticToViewBlend(),
                    t.getBorderScale(), t.getSaturation(), t.getBrightness(), t.showToolbar(),
                    defaultThemeColor, t.getToolbarBackgroundColor(), t.anonymizeToolbar(),
                    R.drawable.textbox, t.getTextBoxBackgroundColor(), t.getTextBoxAlpha(),
                    t.getToolbarAlpha(), t.getToolbarYOffset() * dpToPx, t.getSideBorderScale(),
                    true, t.insetBorderVertical());
        }
        nativeFinishBuildingFrame(mNativePtr);
    }

    /**
     * @return The background color
     */
    protected int getTabListBackgroundColor(Context context) {
        return ApiCompatibilityUtils.getColor(
                context.getResources(), R.color.tab_switcher_background);
    }

    @Override
    protected void initializeNative() {
        if (mNativePtr == 0) {
            mNativePtr = nativeInit();
        }
        assert mNativePtr != 0;
    }

    /**
     * Destroys this object and the corresponding native component.
     */
    @Override
    public void destroy() {
        super.destroy();
        mNativePtr = 0;
    }

    private native long nativeInit();
    private native void nativeBeginBuildingFrame(long nativeTabListSceneLayer);
    private native void nativeFinishBuildingFrame(long nativeTabListSceneLayer);
    private native void nativeUpdateLayer(long nativeTabListSceneLayer, int backgroundColor,
            float viewportX, float viewportY, float viewportWidth, float viewportHeight,
            LayerTitleCache layerTitleCache, TabContentManager tabContentManager,
            ResourceManager resourceManager);
    private native void nativePutTabLayer(long nativeTabListSceneLayer, int id,
            int toolbarResourceId, int closeButtonResourceId, int shadowResourceId,
            int contourResourceId, int backLogoResourceId, int borderResourceId,
            int borderInnerShadowResourceId, boolean canUseLiveLayer, int tabBackgroundColor,
            int backLogoColor, boolean incognito, boolean isPortrait, float x, float y, float width,
            float height, float contentWidth, float contentHeight, float visibleContentHeight,
            float shadowX, float shadowY, float shadowWidth, float shadowHeight, float pivotX,
            float pivotY, float rotationX, float rotationY, float alpha, float borderAlpha,
            float borderInnerShadowAlpha, float contourAlpha, float shadowAlpha, float closeAlpha,
            float closeBtnWidth, float staticToViewBlend, float borderScale, float saturation,
            float brightness, boolean showToolbar, int defaultThemeColor,
            int toolbarBackgroundColor, boolean anonymizeToolbar, int toolbarTextBoxResource,
            int toolbarTextBoxBackgroundColor, float toolbarTextBoxAlpha, float toolbarAlpha,
            float toolbarYOffset, float sideBorderScale, boolean attachContent,
            boolean insetVerticalBorder);
}