package com.seagates3.authserver;

import static java.nio.file.StandardWatchEventKinds.ENTRY_MODIFY;

import java.io.FileNotFoundException;
import java.io.IOException;
import java.nio.file.FileSystems;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.nio.file.WatchEvent;
import java.nio.file.WatchKey;
import java.nio.file.WatchService;
import java.security.GeneralSecurityException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public
class AuthServerConfigChangeListner implements Runnable {

 private
  String configFileName = null;
 private
  String fullFilePath = null;
 private
  WatchService watchService = null;
 private
  static Logger LOGGER =
      LoggerFactory.getLogger(AuthServerConfigChangeListner.class.getName());

 public
  AuthServerConfigChangeListner(final String filePath) {
    this.fullFilePath = filePath;
  }

 public
  void run() {
    try {
      register(this.fullFilePath);
    }
    catch (Exception e) {
      LOGGER.debug("Exception in run method - ", e);
    }
    finally {
      try {
        if (watchService != null) {
          watchService.close();
        }
      }
      catch (IOException e) {
        LOGGER.debug("Exception in finally method - ", e);
      }
    }
  }

 private
  void register(final String file) throws GeneralSecurityException, Exception {
    final int lastIndex = file.lastIndexOf("/");
    String dirPath = file.substring(0, lastIndex + 1);
    String fileName = file.substring(lastIndex + 1, file.length());
    this.configFileName = fileName;

    configurationChanged(file);
    startWatcher(dirPath, fileName);
  }

 private
  void startWatcher(String dirPath, String file) throws IOException {
    watchService = FileSystems.getDefault().newWatchService();
    try {

      Path path = Paths.get(dirPath);
      path.register(watchService, ENTRY_MODIFY);

      WatchKey key = null;
      while (true) {
        key = watchService.take();
        for (WatchEvent < ? > event : key.pollEvents()) {
          if (event.context().toString().equals(configFileName)) {
            configurationChanged(dirPath + file);
          }
        }
        boolean reset = key.reset();
        if (!reset) {
          LOGGER.info("Could not reset the watch key.");
          break;
        }
      }
    }
    catch (InterruptedException ie) {
      LOGGER.info("InterruptedException in watcherservice - " +
                  ie.getMessage());
      Thread.currentThread().interrupt();
    }
    catch (Exception e) {
      LOGGER.info("Exception in watcherservice - " + e.getMessage());
    }
  }

 public
  void configurationChanged(final String file) throws FileNotFoundException,
      IOException, GeneralSecurityException, Exception {
    LOGGER.info("Refreshing the configuration.");
    AuthServerConfig.readConfig(AuthServerConstants.RESOURCE_DIR,
                                "authserver.properties", "keystore.properties");
  }
}
