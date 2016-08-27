package com.smackall.animator.Helper;

import android.graphics.Bitmap;

import org.jcodec.common.model.ColorSpace;
import org.jcodec.common.model.Picture;

import java.nio.IntBuffer;

/**
 * This class is part of JCodec ( www.jcodec.org ) This software is distributed
 * under FreeBSD License
 *
 * @author The JCodec project
 */
public class BitmapUtil {
    private static ThreadLocal<int[]> buffer = new ThreadLocal<int[]>();

    public static Picture fromBitmap(Bitmap src) {
        if (src == null) return null;
        Picture dst = Picture.create(src.getWidth(),
                src.getHeight(), ColorSpace.RGB);
        fromBitmap(src, dst);
        return dst;
    }

    private static void fromBitmap(Bitmap src, Picture dst) {
        int[] dstData = dst.getPlaneData(0);
        int[] packed = new int[src.getWidth() * src.getHeight()];

        src.getPixels(packed, 0, src.getWidth(), 0, 0, src.getWidth(),
                src.getHeight());

        for (int i = 0, srcOff = 0, dstOff = 0; i < src.getHeight(); i++) {
            for (int j = 0; j < src.getWidth(); j++, srcOff++, dstOff += 3) {
                int rgb = packed[srcOff];
                dstData[dstOff] = (rgb >> 16) & 0xff;
                dstData[dstOff + 1] = (rgb >> 8) & 0xff;
                dstData[dstOff + 2] = rgb & 0xff;
            }
        }
    }

    private static void toBitmap(Picture src, Bitmap dst) {
        int[] srcData = src.getPlaneData(0);
        int[] packed = getBuffer(src);

        for (int i = 0, dstOff = 0, srcOff = 0; i < src.getHeight(); i++) {
            for (int j = 0; j < src.getWidth(); j++, dstOff++, srcOff += 3) {
                packed[dstOff] = (srcData[srcOff] << 16)
                        | (srcData[srcOff + 1] << 8) | srcData[srcOff + 2];
            }
        }
        dst.copyPixelsFromBuffer(IntBuffer.wrap(packed));
    }

    private static int[] getBuffer(Picture src) {
        int[] result = buffer.get();
        if (result == null) {
            result = new int[src.getWidth() * src.getHeight()];
            buffer.set(result);
        }
        return result;
    }
}
