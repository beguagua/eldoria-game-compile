package com.raycastcorp.eldoria;

import android.app.NativeActivity;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;

public class EldoriaActivity extends NativeActivity {
    private static final int SHADER_REQUEST = 27815;

    public void openShaderPicker() {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
                intent.addCategory(Intent.CATEGORY_OPENABLE);
                intent.setType("application/octet-stream");
                startActivityForResult(intent, SHADER_REQUEST);
            }
        });
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode != SHADER_REQUEST || resultCode != RESULT_OK || data == null || data.getData() == null) return;
        Uri source = data.getData();
        File destination = new File(getFilesDir(), "eldoria_imported_shader.fs");
        try (InputStream input = getContentResolver().openInputStream(source);
             FileOutputStream output = new FileOutputStream(destination)) {
            if (input == null) return;
            byte[] buffer = new byte[8192];
            int read;
            while ((read = input.read(buffer)) != -1) output.write(buffer, 0, read);
        } catch (Exception ignored) {
            // O jogo mantém o shader compatível caso a cópia falhe ou o usuário cancele.
        }
    }
}
