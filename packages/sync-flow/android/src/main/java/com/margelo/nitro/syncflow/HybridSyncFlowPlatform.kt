package com.margelo.nitro.syncflow

import android.content.Context
import android.security.keystore.KeyGenParameterSpec
import android.security.keystore.KeyProperties
import android.util.Base64
import androidx.annotation.Keep
import com.facebook.proguard.annotations.DoNotStrip
import com.margelo.nitro.NitroModules
import com.margelo.nitro.core.ArrayBuffer
import java.io.File
import java.security.KeyStore
import java.security.SecureRandom
import javax.crypto.Cipher
import javax.crypto.KeyGenerator
import javax.crypto.SecretKey
import javax.crypto.spec.GCMParameterSpec

/**
 * Database keys wrapped by an AES-256-GCM key in the Android Keystore; wrapped bytes live in
 * SharedPreferences (D-05, D-07). The raw key exists only in memory while a database is opened.
 */
@Keep
@DoNotStrip
class HybridSyncFlowPlatform : HybridSyncFlowPlatformSpec() {
  private val context: Context
    get() = NitroModules.applicationContext
      ?: throw IllegalStateException("SF_KEYSTORE: React context is not ready")

  override val databaseDirectory: String
    get() {
      // noBackupFilesDir: keys can't be restored on another device, so backups would be useless.
      val dir = File(context.noBackupFilesDir, "syncflow")
      dir.mkdirs()
      return dir.absolutePath
    }

  override fun getOrCreateKey(alias: String): ArrayBuffer {
    val prefs = context.getSharedPreferences(PREFS, Context.MODE_PRIVATE)
    val stored = prefs.getString(alias, null)
    if (stored != null) {
      return ArrayBuffer.copy(unwrap(Base64.decode(stored, Base64.NO_WRAP)))
    }
    val key = ByteArray(KEY_SIZE).also { SecureRandom().nextBytes(it) }
    val wrapped = wrap(key)
    if (!prefs.edit().putString(alias, Base64.encodeToString(wrapped, Base64.NO_WRAP)).commit()) {
      throw IllegalStateException("SF_KEYSTORE: cannot store key")
    }
    return ArrayBuffer.copy(key)
  }

  override fun deleteKey(alias: String) {
    context.getSharedPreferences(PREFS, Context.MODE_PRIVATE).edit().remove(alias).commit()
  }

  private fun masterKey(): SecretKey {
    val ks = KeyStore.getInstance(ANDROID_KEYSTORE).apply { load(null) }
    (ks.getKey(MASTER_ALIAS, null) as? SecretKey)?.let { return it }
    val gen = KeyGenerator.getInstance(KeyProperties.KEY_ALGORITHM_AES, ANDROID_KEYSTORE)
    gen.init(
      KeyGenParameterSpec.Builder(
        MASTER_ALIAS,
        KeyProperties.PURPOSE_ENCRYPT or KeyProperties.PURPOSE_DECRYPT,
      )
        .setBlockModes(KeyProperties.BLOCK_MODE_GCM)
        .setEncryptionPaddings(KeyProperties.ENCRYPTION_PADDING_NONE)
        .setKeySize(256)
        .build(),
    )
    return gen.generateKey()
  }

  // Layout: [12-byte IV][ciphertext + 16-byte tag]
  private fun wrap(plain: ByteArray): ByteArray {
    val cipher = Cipher.getInstance(TRANSFORMATION)
    cipher.init(Cipher.ENCRYPT_MODE, masterKey())
    return cipher.iv + cipher.doFinal(plain)
  }

  private fun unwrap(blob: ByteArray): ByteArray {
    if (blob.size <= IV_SIZE) throw IllegalStateException("SF_KEYSTORE: stored key is corrupt")
    val cipher = Cipher.getInstance(TRANSFORMATION)
    cipher.init(Cipher.DECRYPT_MODE, masterKey(), GCMParameterSpec(128, blob, 0, IV_SIZE))
    val key = cipher.doFinal(blob, IV_SIZE, blob.size - IV_SIZE)
    if (key.size != KEY_SIZE) throw IllegalStateException("SF_KEYSTORE: stored key has wrong size")
    return key
  }

  private companion object {
    const val PREFS = "com.nikhilll.syncflow.keys"
    const val ANDROID_KEYSTORE = "AndroidKeyStore"
    const val MASTER_ALIAS = "com.nikhilll.syncflow.master"
    const val TRANSFORMATION = "AES/GCM/NoPadding"
    const val KEY_SIZE = 32
    const val IV_SIZE = 12
  }
}
