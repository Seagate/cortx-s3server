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

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.Properties;
import java.util.logging.FileHandler;
import java.util.logging.Level;
import java.util.logging.Logger;
import java.util.logging.SimpleFormatter;

import io.netty.bootstrap.ServerBootstrap;
import io.netty.channel.Channel;
import io.netty.channel.ChannelOption;
import io.netty.channel.EventLoopGroup;
import io.netty.channel.nio.NioEventLoopGroup;
import io.netty.channel.socket.nio.NioServerSocketChannel;
import io.netty.handler.logging.LogLevel;
import io.netty.handler.logging.LoggingHandler;

import com.seagates3.dao.DAODispatcher;
import com.seagates3.dao.ldap.LdapConnectionManager;
import com.seagates3.util.SAMLUtil;




public class AuthServer {

    static Properties authServerConfig;
    static final Logger LOGGER = Logger.getLogger("authLog");

    /**
     * Read authserver.properties and load the properties in
     */
    static void readConfig() throws FileNotFoundException, IOException {
        Path authProperties = Paths.get("", "resources", "authserver.properties");
        authServerConfig = new Properties();
        InputStream input = new FileInputStream(authProperties.toString());
        authServerConfig.load(input);
    }

    /*
     * Create a Filehandler for Logger.
     * Set level to ALL.
     */
    static void logInit() throws IOException {
        // Create LogFile if it doesn't exist
        File logFile = new File(authServerConfig.getProperty("logFilePath"), "auth.log");
        if(!logFile.exists()) {
            logFile.createNewFile();
        }

        FileHandler fh = new FileHandler(logFile.toString(), 1048576, 3, true);
        fh.setFormatter(new SimpleFormatter());
        LOGGER.addHandler(fh);
        LOGGER.setLevel(Level.ALL);
    }

    public static void main(String[] args) throws IOException {
        readConfig();
        logInit();
        DAODispatcher.Init(authServerConfig.getProperty("dataSource"));
        LdapConnectionManager.initLdap(authServerConfig);
        AuthServerConfig.init(authServerConfig);
        SAMLUtil.init();

        // Configure the server.
        EventLoopGroup bossGroup = new NioEventLoopGroup(1);
        EventLoopGroup workerGroup = new NioEventLoopGroup();
        try {
            ServerBootstrap b = new ServerBootstrap();
            b.option(ChannelOption.SO_BACKLOG, 1024);
            b.group(bossGroup, workerGroup)
             .channel(NioServerSocketChannel.class)
             .handler(new LoggingHandler(LogLevel.INFO))
             .childHandler(new AuthServerInitializer());

            Channel ch = b.bind(Integer.parseInt(authServerConfig.getProperty("port")))
                    .sync().channel();

            ch.closeFuture().sync();
        } catch (InterruptedException ex) {
            Logger.getLogger(AuthServer.class.getName()).log(Level.SEVERE, null, ex);
        } finally {
            bossGroup.shutdownGracefully();
            workerGroup.shutdownGracefully();
        }
    }
}
