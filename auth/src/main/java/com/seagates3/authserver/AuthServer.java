/*
 * COPYRIGHT 2015 SEAGATE LLC
 *
 * THIS DRAWING/DOCUMENT, ITS SPECIFICATIONS, AND THE DATA CONTAINED
 * HEREIN, ARE THE EXCLUSIVE PROPERTY OF SEAGATE TECHNOLOGY
 * LIMITED, ISSUED IN STRICT CONFIDENCE AND SHALL NOT, WITHOUT
 * THE PRIOR WRITTEN PERMISSION OF SEAGATE TECHNOLOGY LIMITED,
 * BE REPRODUCED, COPIED, OR DISCLOSED TO A THIRD PARTY, OR
 * USED FOR ANY PURPOSE WHATSOEVER, OR STORED IN A RETRIEVAL SYSTEM
 * EXCEPT AS ALLOWED BY THE TERMS OF SEAGATE LICENSES AND AGREEMENTS.
 *
 * YOU SHOULD HAVE RECEIVED A COPY OF SEAGATE'S LICENSE ALONG WITH
 * THIS RELEASE. IF NOT PLEASE CONTACT A SEAGATE REPRESENTATIVE
 * http://www.seagate.com/contact
 *
 * Original author:  Arjun Hariharan <arjun.hariharan@seagate.com>
 * Original creation date: 17-Sep-2014
 */
package com.seagates3.authserver;

import com.seagates3.dao.DAODispatcher;
import io.netty.bootstrap.ServerBootstrap;
import io.netty.channel.Channel;
import io.netty.channel.ChannelOption;
import io.netty.channel.EventLoopGroup;
import io.netty.channel.nio.NioEventLoopGroup;
import io.netty.channel.socket.nio.NioServerSocketChannel;
import io.netty.handler.logging.LogLevel;
import io.netty.handler.logging.LoggingHandler;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.Properties;
import java.util.logging.FileHandler;
import java.util.logging.Level;
import java.util.logging.Logger;
import java.util.logging.SimpleFormatter;

public class AuthServer {

    static Properties authServerConfig;
    static final Logger LOGGER = Logger.getLogger("authLog");

    /**
     * Read the properties file.
     */
    static void readConfig() throws FileNotFoundException, IOException {
        Path authProperties = Paths.get("", "resources", "authserver.properties");
        authServerConfig = new Properties();
        InputStream input = new FileInputStream(authProperties.toString());
        authServerConfig.load(input);
    }

    /**
     * Create a File handler for Logger. Set level to ALL.
     */
    static void logInit() throws IOException {
        File logFile = new File(authServerConfig.getProperty("logFilePath"),
                "auth.log");
        if (!logFile.exists()) {
            logFile.createNewFile();
        }

        FileHandler fh = new FileHandler(logFile.toString(), 1048576, 3, true);
        fh.setFormatter(new SimpleFormatter());
        LOGGER.addHandler(fh);
        LOGGER.setLevel(Level.ALL);
    }

    public static void main(String[] args) throws Exception {
        readConfig();
        logInit();

        AuthServerConfig.init(authServerConfig);
        LOGGER.info("Auth server configured.");

        SSLContextProvider.init();
        LOGGER.info("SSL Context provider initialized.");

        DAODispatcher.Init();
        LOGGER.info("Database initialized.");

        // Configure the server.
        EventLoopGroup bossGroup = new NioEventLoopGroup(1);
        EventLoopGroup workerGroup = new NioEventLoopGroup(4);

        ArrayList<Channel> serverChannels = new ArrayList<>();
        int httpPort = AuthServerConfig.getHttpPort();
        Channel serverChannel = httpServerBootstrap(bossGroup, workerGroup,
                httpPort);
        serverChannels.add(serverChannel);

        if (AuthServerConfig.isHttpsEnabled()) {
            int httpsPort = AuthServerConfig.getHttpsPort();
            serverChannel = httpsServerBootstrap(bossGroup, workerGroup,
                    httpsPort);
            serverChannels.add(serverChannel);
        }

        for (Channel ch : serverChannels) {
            ch.closeFuture().sync();
        }
    }

    /**
     * Create a new ServerBootstrap for HTTP protocol.
     *
     * @param port Http port.
     */
    private static Channel httpServerBootstrap(EventLoopGroup bossGroup,
            EventLoopGroup workerGroup, int port) {
        try {
            ServerBootstrap b = new ServerBootstrap();
            b.option(ChannelOption.SO_BACKLOG, 1024);
            b.group(bossGroup, workerGroup)
                    .channel(NioServerSocketChannel.class)
                    .handler(new LoggingHandler(LogLevel.INFO))
                    .childHandler(new AuthServerHTTPInitializer());

            Channel serverChannel = b.bind(port).sync().channel();
            LOGGER.info("Auth server(http) is running.");
            return serverChannel;

        } catch (InterruptedException ex) {
            Logger.getLogger(AuthServer.class.getName()).log(Level.SEVERE,
                    null, ex);
        }
        return null;
    }

    private static Channel httpsServerBootstrap(EventLoopGroup bossGroup,
            EventLoopGroup workerGroup, int port) {
        try {
            ServerBootstrap b = new ServerBootstrap();
            b.option(ChannelOption.SO_BACKLOG, 1024);
            b.group(bossGroup, workerGroup)
                    .channel(NioServerSocketChannel.class)
                    .handler(new LoggingHandler(LogLevel.INFO))
                    .childHandler(new AuthServerHTTPSInitializer());

            Channel serverChannel = b.bind(port).sync().channel();
            LOGGER.info("Auth server (https) is running.");
            return serverChannel;
        } catch (InterruptedException ex) {
            Logger.getLogger(AuthServer.class.getName())
                    .log(Level.SEVERE, null, ex);
        }

        return null;
    }
}
