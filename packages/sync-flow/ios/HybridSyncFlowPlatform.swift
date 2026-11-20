import Foundation
import NitroModules
import Security

/// Keychain-backed key storage and the database directory (D-05, D-07).
final class HybridSyncFlowPlatform: HybridSyncFlowPlatformSpec {
  private static let service = "com.nikhilll.syncflow.keys"
  private static let keySize = 32

  var databaseDirectory: String {
    get {
      let base = FileManager.default.urls(for: .applicationSupportDirectory, in: .userDomainMask)[0]
      var dir = base.appendingPathComponent("syncflow", isDirectory: true)
      try? FileManager.default.createDirectory(at: dir, withIntermediateDirectories: true)
      // Keys are ThisDeviceOnly, so a backed-up database could never be opened anyway.
      var values = URLResourceValues()
      values.isExcludedFromBackup = true
      try? dir.setResourceValues(values)
      return dir.path
    }
  }

  func getOrCreateKey(alias: String) throws -> ArrayBuffer {
    if let existing = try Self.read(alias: alias) {
      return try ArrayBuffer.copy(data: existing)
    }
    var bytes = Data(count: Self.keySize)
    let status = bytes.withUnsafeMutableBytes {
      SecRandomCopyBytes(kSecRandomDefault, Self.keySize, $0.baseAddress!)
    }
    guard status == errSecSuccess else {
      throw RuntimeError.error(withMessage: "SF_KEYSTORE: random key generation failed (\(status))")
    }
    try Self.store(alias: alias, key: bytes)
    return try ArrayBuffer.copy(data: bytes)
  }

  func deleteKey(alias: String) throws {
    let status = SecItemDelete(Self.query(alias: alias) as CFDictionary)
    guard status == errSecSuccess || status == errSecItemNotFound else {
      throw RuntimeError.error(withMessage: "SF_KEYSTORE: cannot delete key (\(status))")
    }
  }

  private static func query(alias: String) -> [String: Any] {
    [
      kSecClass as String: kSecClassGenericPassword,
      kSecAttrService as String: service,
      kSecAttrAccount as String: alias,
    ]
  }

  private static func read(alias: String) throws -> Data? {
    var q = query(alias: alias)
    q[kSecReturnData as String] = true
    q[kSecMatchLimit as String] = kSecMatchLimitOne
    var out: AnyObject?
    let status = SecItemCopyMatching(q as CFDictionary, &out)
    if status == errSecItemNotFound { return nil }
    guard status == errSecSuccess, let data = out as? Data, data.count == keySize else {
      throw RuntimeError.error(withMessage: "SF_KEYSTORE: cannot read key (\(status))")
    }
    return data
  }

  private static func store(alias: String, key: Data) throws {
    var q = query(alias: alias)
    q[kSecValueData as String] = key
    // Readable after the first unlock so background sync works while locked; never synced or
    // restored to another device.
    q[kSecAttrAccessible as String] = kSecAttrAccessibleAfterFirstUnlockThisDeviceOnly
    let status = SecItemAdd(q as CFDictionary, nil)
    guard status == errSecSuccess else {
      throw RuntimeError.error(withMessage: "SF_KEYSTORE: cannot store key (\(status))")
    }
  }
}
