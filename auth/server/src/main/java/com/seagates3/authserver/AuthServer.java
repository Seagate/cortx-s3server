/*
 * Copyright (c) 2020 Seagate Technology LLC and/or its Affiliates
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * For any questions about this software or licensing,
 * please email opensource@seagate.com or cortx-questions@seagate.com.
 *
 */

package com.seagates3.authserver;

import com.seagates3.dao.DAODispatcher;
import com.seagates3.exception.ServerInitialisationException;
import com.seagates3.fi.FaultPoints;
import com.seagates3.perf.S3Perf;
import io.netty.bootstrap.ServerBootstrap;
import io.netty.channel.Channel;
import io.netty.channel.ChannelOption;
import io.netty.channel.EventLoopGroup;
import io.netty.channel.nio.NioEventLoopGroup;
import io.netty.channel.socket.nio.NioServerSocketChannel;
import io.netty.handler.logging.LogLevel;
import io.netty.handler.logging.LoggingHandler;
import io.netty.util.concurrent.DefaultEventExecutorGroup;
import io.netty.util.concurrent.EventExecutorGroup;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.security.GeneralSecurityException;
import java.util.ArrayList;
import org.apache.logging.log4j.Level;
import org.apache.logging.log4j.core.config.ConfigurationSource;
import org.apache.logging.log4j.core.config.Configurator;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public
class AuthServer {

 private
  static EventLoopGroup bossGroup, workerGroup;
 private
  static EventExecutorGroup executorGroup;

 private
  static Logger logger;

 private
  static void attachShutDownHook() {
        Runtime.getRuntime().addShutdownHook(new Thread() {
            @Override
            public void run() {
                S3Perf.clean();
                shutdownExecutors();
  }
        });
        }

       private
        static void shutdownExecutors() {
          bossGroup.shutdownGracefully();
          logger.info("Boss group shutdown");

          workerGroup.shutdownGracefully();
          logger.info("Worker group shutdown");

          executorGroup.shutdownGracefully();
          logger.info("Executor group shutdown");
        }

        /**
         * Create a File handler for Logger. Set level to ALL.
         */
        static void logInit() throws IOException,
            ServerInitialisationException {
          String logConfigFilePath = AuthServerConfig.getLogConfigFile();

          /**
           * If log4j config file is given, override the default Logging
           * properties file.
           */
          if (logConfigFilePath != null) {
            File logConfigFile = new File(AuthServerConfig.getLogConfigFile());
            if (logConfigFile.exists()) {
              FileInputStream fis = new FileInputStream(logConfigFile);
              ConfigurationSource source = new ConfigurationSource(fis);
              Configurator.initialize(null, source);
              if (fis != null) fis.close();
            } else {
              throw new ServerInitialisationException(
                  "Logging config file doesn't exist.");
            }
          }

          String logLevel = AuthServerConfig.getLogLevel();
          if (logLevel != null) {
            Level level = Level.getLevel(logLevel);
            if (level == null) {
              throw new ServerInitialisationException(
                  "Incorrect logging level.");
            } else {
              Configurator.setRootLevel(level);
            }
          }
        }

       public
        static void main(String[] args) throws InterruptedException,
            IOException, ServerInitialisationException,
            GeneralSecurityException, Exception {

          AuthServerConfig.readConfig(AuthServerConstants.RESOURCE_DIR,
                                      "authserver.properties",
                                      "keystore.properties");
          logInit();
          // LoggerFactory.getLogger works after this logInit() call.
          AuthServerConfig.logConfigProps();
          AuthServerConfig.loadCredentials();

          logger = LoggerFactory.getLogger(AuthServer.class.getName());

          if (!(AuthServerConfig.isHttpEnabled() ||
                AuthServerConfig.isHttpsEnabled())) {
            logger.error(
                "Both HTTP and HTTPS are disabled. At least one channel " +
                "should be enabled.");
            System.exit(1);
          }

          SSLContextProvider.init();
          IAMResourceMapper.init();
          DAODispatcher.init();
          S3Perf.init();

          /**
           * If fault injection is enabled, create instance of FaultPoints
           */
          if (AuthServerConfig.isFaultInjectionEnabled()) {
            FaultPoints.init();
          }

          AuthServer.attachShutDownHook();

          // Configure the server.
          bossGroup =
              new NioEventLoopGroup(AuthServerConfig.getBossGroupThreads());
          logger.info("Created boss event loop group with " +
                      AuthServerConfig.getBossGroupThreads() + " threads");

          workerGroup =
              new NioEventLoopGroup(AuthServerConfig.getWorkerGroupThreads());
          logger.info("Created worker event loop group with " +
                      AuthServerConfig.getWorkerGroupThreads() + " threads");

          executorGroup = new DefaultEventExecutorGroup(
              AuthServerConfig.getEventExecutorThreads());
          logger.info("Created event executor with " +
                      AuthServerConfig.getEventExecutorThreads() + " threads");

          ArrayList<Channel> serverChannels = new ArrayList<>();

          if (AuthServerConfig.isHttpEnabled()) {
            int httpPort = AuthServerConfig.getHttpPort();
            String host = AuthServerConfig.getDefaultHost();
            Channel serverChannel = httpServerBootstrap(
                bossGroup, workerGroup, executorGroup, host, httpPort);
            serverChannels.add(serverChannel);
            logger.info("Auth server is listening on HTTP port " + httpPort);
          }

          if (AuthServerConfig.isHttpsEnabled()) {
            int httpsPort = AuthServerConfig.getHttpsPort();
            String host = AuthServerConfig.getDefaultHost();
            Channel serverChannel = httpsServerBootstrap(
                bossGroup, workerGroup, executorGroup, host, httpsPort);
            serverChannels.add(serverChannel);
            logger.info("Auth server is listening on HTTPS port " + httpsPort);
          }

          for (Channel ch : serverChannels) {
            ch.closeFuture().sync();
          }
        }

        /**
         * Create a new ServerBootstrap for HTTP protocol.
         *
         * @param port HTTP port.
         */
       private
        static Channel httpServerBootstrap(
            EventLoopGroup bossGroup, EventLoopGroup workerGroup,
            EventExecutorGroup executorGroup, String host,
            int port) throws InterruptedException {
          ServerBootstrap b = new ServerBootstrap();
          b.option(ChannelOption.SO_BACKLOG, 1024);
          b.group(bossGroup, workerGroup)
              .channel(NioServerSocketChannel.class)
              .handler(new LoggingHandler(LogLevel.INFO))
              .childHandler(new AuthServerHTTPInitializer(executorGroup));

          Channel serverChannel = b.bind(host, port).sync().channel();
          return serverChannel;
        }

       private
        static Channel httpsServerBootstrap(
            EventLoopGroup bossGroup, EventLoopGroup workerGroup,
            EventExecutorGroup executorGroup, String host,
            int port) throws InterruptedException {
          ServerBootstrap b = new ServerBootstrap();
          b.option(ChannelOption.SO_BACKLOG, 1024);
          b.group(bossGroup, workerGroup)
              .channel(NioServerSocketChannel.class)
              .handler(new LoggingHandler(LogLevel.INFO))
              .childHandler(new AuthServerHTTPSInitializer(executorGroup));

          Channel serverChannel = b.bind(host, port).sync().channel();
          return serverChannel;
        }
        }
