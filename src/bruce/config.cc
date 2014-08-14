/* <bruce/config.cc>

   ----------------------------------------------------------------------------
   Copyright 2013-2014 Tagged

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
   ----------------------------------------------------------------------------

   Implements <bruce/config.h>.
 */

#include <bruce/config.h>

#include <vector>

#include <libgen.h>
#include <syslog.h>

#include <base/basename.h>
#include <bruce/util/arg_parse_error.h>
#include <bruce/version.h>
#include <tclap/CmdLine.h>

using namespace Base;
using namespace Bruce;
using namespace Bruce::Util;

static void ParseArgs(int argc, char *argv[], TConfig &config) {
  using namespace TCLAP;
  const std::string prog_name = Basename(argv[0]);
  std::vector<const char *> arg_vec(&argv[0], &argv[0] + argc);
  arg_vec[0] = prog_name.c_str();

  try {
    CmdLine cmd("Producer daemon for Apache Kafka", ' ', GetVersion());
    ValueArg<decltype(config.ConfigPath)> arg_config_path("", "config_path",
        "Pathname of config file.", true, config.ConfigPath, "PATH");
    cmd.add(arg_config_path);
    SwitchArg arg_discard_all_data("", "discard_all_data",
        "Discard all data rather than sending to Kafka.  Since all data is "
        "being explicitly discarded, no discard reporting or logging is "
        "performed.", cmd, config.DiscardAllData);
    SwitchArg arg_log_echo("", "log_echo", "Echo syslog messages to standard "
        "error.", cmd, config.LogEcho);
    ValueArg<decltype(config.ReceiveSocketName)> arg_receive_socket_name("",
        "receive_socket_name", "Pathname of UNIX domain socket for receiving "
        "messages from web clients", true, config.ReceiveSocketName, "PATH");
    cmd.add(arg_receive_socket_name);
    ValueArg<decltype(config.ProtocolVersion)> arg_protocol_version("",
        "protocol_version", "Version of Kafka protocol to use.", true,
        config.ProtocolVersion, "VERSION");
    cmd.add(arg_protocol_version);
    ValueArg<decltype(config.StatusPort)> arg_status_port("", "status_port",
        "HTTP Status monitoring port.", false, config.StatusPort, "PORT");
    cmd.add(arg_status_port);
    ValueArg<decltype(config.MsgBufferMax)> arg_msg_buffer_max("",
        "msg_buffer_max", "Maximum amount of memory in Kb to use for "
        "buffering messages.", true, config.MsgBufferMax, "MAX_KB");
    cmd.add(arg_msg_buffer_max);
    ValueArg<decltype(config.MaxInputMsgSize)> arg_max_input_msg_size("",
        "max_input_msg_size", "Maximum input message size in bytes expected "
        "from clients.  Messages larger than this value will be discarded.  "
        "Here, \"size\" means total combined size of topic, key, and value.",
        false, config.MaxInputMsgSize, "MAX_BYTES");
    cmd.add(arg_max_input_msg_size);
    SwitchArg arg_allow_large_unix_datagrams("", "allow_large_unix_datagrams",
        "Allow large enough values for max_input_msg_size that a client "
        "sending a UNIX domain datagram of the maximum allowed size will need "
        "to increase its SO_SNDBUF socket option above the default value.",
        cmd, config.AllowLargeUnixDatagrams);
    ValueArg<decltype(config.MaxFailedDeliveryAttempts)>
        arg_max_failed_delivery_attempts("", "max_failed_delivery_attempts",
        "Maximum number of failed delivery attempts allowed before a message "
        "is discarded.", false, config.MaxFailedDeliveryAttempts,
        "MAX_ATTEMPTS");
    cmd.add(arg_max_failed_delivery_attempts);
    SwitchArg arg_daemon("", "daemon", "Run as daemon.", cmd, config.Daemon);
    ValueArg<decltype(config.ClientId)> arg_client_id("", "client_id",
        "Client ID string to send in produce requests.", false,
        config.ClientId, "CLIENT_ID");
    cmd.add(arg_client_id);
    ValueArg<decltype(config.RequiredAcks)> arg_required_acks("",
        "required_acks", "Required ACKs value to send in produce requests.",
        false, config.RequiredAcks, "REQUIRED_ACKS");
    cmd.add(arg_required_acks);
    ValueArg<decltype(config.ReplicationTimeout)> arg_replication_timeout("",
        "replication_timeout", "Replication timeout value to send in produce "
        "requests.", false, config.ReplicationTimeout, "TIMEOUT");
    cmd.add(arg_replication_timeout);
    ValueArg<decltype(config.ShutdownMaxDelay)> arg_shutdown_max_delay("",
        "shutdown_max_delay", "Maximum delay in milliseconds for sending "
        "buffered messages once shutdown signal is received.", false,
        config.ShutdownMaxDelay, "MAX_DELAY_MS");
    cmd.add(arg_shutdown_max_delay);
    ValueArg<decltype(config.DispatcherRestartMaxDelay)>
        arg_dispatcher_restart_max_delay("", "dispatcher_restart_max_delay",
        "Max dispatcher shutdown delay in milliseconds when restarting "
        "dispatcher for metadata update", false,
        config.DispatcherRestartMaxDelay, "MAX_DELAY_MS");
    cmd.add(arg_dispatcher_restart_max_delay);
    ValueArg<decltype(config.MetadataRefreshInterval)>
        arg_metadata_refresh_interval("", "metadata_refresh_interval",
        "Interval in minutes (plus or minus a bit of randomness) between "
        "periodic metadata updates", false, config.MetadataRefreshInterval,
        "INTERVAL_MINUTES");
    cmd.add(arg_metadata_refresh_interval);
    ValueArg<decltype(config.KafkaSocketTimeout)> arg_kafka_socket_timeout("",
        "kafka_socket_timeout", "Socket timeout in seconds to use when "
        "communicating with Kafka broker.", false, config.KafkaSocketTimeout,
        "TIMEOUT_SECONDS");
    cmd.add(arg_kafka_socket_timeout);
    ValueArg<decltype(config.PauseRateLimitInitial)>
        arg_pause_rate_limit_initial("", "pause_rate_limit_initial", "Initial "
        "delay value in milliseconds between consecutive metadata fetches due "
        "to Kafka-related errors.  The actual value has some randomness "
        "added.", false, config.PauseRateLimitInitial, "DELAY_MS");
    cmd.add(arg_pause_rate_limit_initial);
    ValueArg<decltype(config.PauseRateLimitMaxDouble)>
        arg_pause_rate_limit_max_double("", "pause_rate_limit_max_double",
        "Maximum number of times to double pause_rate_limit_initial on "
        "repeated errors.", false, config.PauseRateLimitMaxDouble,
        "MAX_DOUBLE");
    cmd.add(arg_pause_rate_limit_max_double);
    ValueArg<decltype(config.MinPauseDelay)> arg_min_pause_delay("",
        "min_pause_delay", "Minimum delay in milliseconds before fetching new "
        "metadata from Kafka in response to an error.", false,
        config.MinPauseDelay, "MIN_DELAY_MS");
    cmd.add(arg_min_pause_delay);
    SwitchArg arg_omit_timestamp("", "omit_timestamp", "Omit timestamps from "
        "messages (applicable only when using legacy input format).  Do not "
        "use this option, since it will soon be removed.  Its purpose is to "
        "provide compatibility with legacy infrastructure at Tagged.", cmd,
        config.OmitTimestamp);
    ValueArg<decltype(config.DiscardReportInterval)>
        arg_discard_report_interval("", "discard_report_interval",
        "Discard reporting interval in seconds.", false,
        config.DiscardReportInterval, "INTERVAL_SECONDS");
    cmd.add(arg_discard_report_interval);
    ValueArg<decltype(config.DebugDir)> arg_debug_dir("", "debug_dir",
        "Directory for debug instrumentation files.", false, config.DebugDir,
        "DIR");
    cmd.add(arg_debug_dir);
    ValueArg<decltype(config.MsgDebugTimeLimit)> arg_msg_debug_time_limit("",
        "msg_debug_time_limit", "Message debugging time limit in seconds.",
        false, config.MsgDebugTimeLimit, "LIMIT_SECONDS");
    cmd.add(arg_msg_debug_time_limit);
    ValueArg<decltype(config.MsgDebugByteLimit)> arg_msg_debug_byte_limit("",
        "msg_debug_byte_limit", "Message debugging byte limit.", false,
        config.MsgDebugByteLimit, "MAX_BYTES");
    cmd.add(arg_msg_debug_byte_limit);
    SwitchArg arg_compare_metadata_on_refresh("",
        "compare_metadata_on_refresh", "On metadata refresh, compare new "
        "metadata to old metadata, and discard new metadata if they are the "
        "same.  This should be set to true for normal operation, but setting "
        "it to false may be useful for testing.", cmd,
        config.CompareMetadataOnRefresh);
    ValueArg<decltype(config.DiscardLogPath)> arg_discard_log_path("",
        "discard_log_path", "Absolute pathname of local file where discards "
        "will be logged.  This is intended for debugging.  If unspecified, "
        "logging of discards to a file will be disabled.", false,
        config.DiscardLogPath, "PATH");
    cmd.add(arg_discard_log_path);
    ValueArg<decltype(config.DiscardLogMaxFileSize)>
        arg_discard_log_max_file_size("", "discard_log_max_file_size",
              "Maximum size (in Kb) of discard logfile.  When the next log "
              "entry e would exceed the maximum, the logfile (with name f) is "
              "renamed to f.N wnere N is the current time in milliseconds "
              "since the epoch.  Then a new file f is opened, and e is "
              "written to f.  See also discard_log_max_archive_size.", false,
              config.DiscardLogMaxFileSize, "MAX_KB");
    cmd.add(arg_discard_log_max_file_size);
    ValueArg<decltype(config.DiscardLogMaxArchiveSize)>
        arg_discard_log_max_archive_size("", "discard_log_max_archive_size",
        "See description of discard_log_max_file_size.  Once a discard "
        "logfile is renamed from f to f.N due to the size restriction imposed "
        "by discard_log_max_file_size, the directory containing f.N is "
        "scanned for all old discard logfiles.  If their combined size "
        "exceeds discard_log_max_archive_size (specified in Kb), then old "
        "logfiles are deleted, starting with the oldest, until their combined "
        "size no longer exceeds the maximum.", false,
        config.DiscardLogMaxArchiveSize, "MAX_KB");
    cmd.add(arg_discard_log_max_archive_size);
    ValueArg<decltype(config.DiscardLogBadMsgPrefixSize)>
        arg_discard_log_bad_msg_prefix_size("",
        "discard_log_bad_msg_prefix_size", "Maximum bad message prefix size "
        "in bytes to write to discard logfile when discarding", false,
        config.DiscardLogBadMsgPrefixSize, "MAX_BYTES");
    cmd.add(arg_discard_log_bad_msg_prefix_size);
    ValueArg<decltype(config.DiscardReportBadMsgPrefixSize)>
        arg_discard_report_bad_msg_prefix_size("",
        "discard_report_bad_msg_prefix_size", "Maximum bad message prefix "
        "size in bytes to write to discard report", false,
        config.DiscardReportBadMsgPrefixSize, "MAX_BYTES");
    cmd.add(arg_discard_report_bad_msg_prefix_size);
    SwitchArg arg_use_old_input_format("", "use_old_input_format", "Expect "
        "input UNIX datagrams to adhere to old format.  Do not use this "
        "option, since it will soon be removed.  Its purpose is to provide "
        "compatibility with legacy infrastructure at Tagged.", cmd,
        config.UseOldInputFormat);
    SwitchArg arg_use_old_output_format("", "use_old_output_format", "Send "
        "messages to Kafka using old format.  Do not use this option, since "
        "it will soon be removed.  Its purpose is to provide compatibility "
        "with legacy infrastructure at Tagged.", cmd,
        config.UseOldOutputFormat);
    cmd.parse(argc, &arg_vec[0]);
    config.ConfigPath = arg_config_path.getValue();
    config.DiscardAllData = arg_discard_all_data.getValue();
    config.LogEcho = arg_log_echo.getValue();
    config.ReceiveSocketName = arg_receive_socket_name.getValue();
    config.ProtocolVersion = arg_protocol_version.getValue();
    config.StatusPort = arg_status_port.getValue();
    config.MsgBufferMax = arg_msg_buffer_max.getValue();
    config.MaxInputMsgSize = arg_max_input_msg_size.getValue();
    config.AllowLargeUnixDatagrams = arg_allow_large_unix_datagrams.getValue();
    config.MaxFailedDeliveryAttempts =
        arg_max_failed_delivery_attempts.getValue();
    config.Daemon = arg_daemon.getValue();
    config.ClientId = arg_client_id.getValue();
    config.RequiredAcks = arg_required_acks.getValue();
    config.ReplicationTimeout = arg_replication_timeout.getValue();
    config.ShutdownMaxDelay = arg_shutdown_max_delay.getValue();
    config.DispatcherRestartMaxDelay =
        arg_dispatcher_restart_max_delay.getValue();
    config.MetadataRefreshInterval = arg_metadata_refresh_interval.getValue();
    config.KafkaSocketTimeout = arg_kafka_socket_timeout.getValue();
    config.PauseRateLimitInitial = arg_pause_rate_limit_initial.getValue();
    config.PauseRateLimitMaxDouble =
        arg_pause_rate_limit_max_double.getValue();
    config.MinPauseDelay = arg_min_pause_delay.getValue();
    config.OmitTimestamp = arg_omit_timestamp.getValue();
    config.DiscardReportInterval = arg_discard_report_interval.getValue();
    config.DebugDir = arg_debug_dir.getValue();
    config.MsgDebugTimeLimit = arg_msg_debug_time_limit.getValue();
    config.MsgDebugByteLimit = arg_msg_debug_byte_limit.getValue();
    config.CompareMetadataOnRefresh =
        arg_compare_metadata_on_refresh.getValue();
    config.DiscardLogPath = arg_discard_log_path.getValue();
    config.DiscardLogMaxFileSize = arg_discard_log_max_file_size.getValue();
    config.DiscardLogMaxArchiveSize =
        arg_discard_log_max_archive_size.getValue();
    config.DiscardLogBadMsgPrefixSize =
        arg_discard_log_bad_msg_prefix_size.getValue();
    config.DiscardReportBadMsgPrefixSize =
        arg_discard_report_bad_msg_prefix_size.getValue();
    config.UseOldInputFormat = arg_use_old_input_format.getValue();
    config.UseOldOutputFormat = arg_use_old_output_format.getValue();
  } catch (const ArgException &x) {
    throw TArgParseError(x.error(), x.argId());
  }
}

TConfig::TConfig(int argc, char *argv[])
    : DiscardAllData(false),
      LogEcho(false),
      ProtocolVersion(0),
      StatusPort(9090),
      MsgBufferMax(256 * 1024),
      MaxInputMsgSize(128 * 1024),
      AllowLargeUnixDatagrams(false),
      MaxFailedDeliveryAttempts(5),
      Daemon(false),
      RequiredAcks(-1),
      ReplicationTimeout(10000),
      ShutdownMaxDelay(30000),
      DispatcherRestartMaxDelay(5000),
      MetadataRefreshInterval(15),
      KafkaSocketTimeout(60),
      PauseRateLimitInitial(5000),
      PauseRateLimitMaxDouble(4),
      MinPauseDelay(5000),
      OmitTimestamp(false),
      DiscardReportInterval(600),
      DebugDir("/home/bruce/debug"),
      MsgDebugTimeLimit(3600),
      MsgDebugByteLimit(2UL * 1024UL * 1024UL * 1024UL),
      CompareMetadataOnRefresh(true),
      DiscardLogMaxFileSize(1024),
      DiscardLogMaxArchiveSize(8 * 1024),
      DiscardLogBadMsgPrefixSize(256),
      DiscardReportBadMsgPrefixSize(256),
      UseOldInputFormat(false),
      UseOldOutputFormat(false) {
  ParseArgs(argc, argv, *this);
}

void Bruce::LogConfig(const TConfig &config) {
  syslog(LOG_INFO, "Version: [%s]", GetVersion());
  syslog(LOG_INFO, "Config file: [%s]", config.ConfigPath.c_str());

  if (config.DiscardAllData) {
    syslog(LOG_WARNING, "Discarding all data as requested");
  }

  syslog(LOG_INFO, "UNIX domain datagram input socket [%s]",
         config.ReceiveSocketName.c_str());
  syslog(LOG_INFO, "Using Kafka protocol version [%lu]",
         static_cast<unsigned long>(config.ProtocolVersion));
  syslog(LOG_INFO, "Listening on status port %u",
         static_cast<unsigned>(config.StatusPort));
  syslog(LOG_INFO, "Buffered message limit %lu kbytes",
         static_cast<unsigned long>(config.MsgBufferMax));
  syslog(LOG_INFO, "Max input message size %lu bytes",
         static_cast<unsigned long>(config.MaxInputMsgSize));
  syslog(LOG_INFO, "Allow large UNIX datagrams: %s",
         config.AllowLargeUnixDatagrams ? "true" : "false");
  syslog(LOG_INFO, "Max failed delivery attempts %lu",
         static_cast<unsigned long>(config.MaxFailedDeliveryAttempts));
  syslog(LOG_INFO, config.Daemon ?
         "Running as daemon" : "Not running as daemon");
  syslog(LOG_INFO, "Client ID [%s]", config.ClientId.c_str());
  syslog(LOG_INFO, "Required ACKs %d",
         static_cast<int>(config.RequiredAcks));
  syslog(LOG_INFO, "Replication timeout %d milliseconds",
         static_cast<int>(config.ReplicationTimeout));
  syslog(LOG_INFO, "Shutdown send grace period %lu milliseconds",
         static_cast<unsigned long>(config.ShutdownMaxDelay));
  syslog(LOG_INFO, "Kafka dispatch restart grace period %lu milliseconds",
         static_cast<unsigned long>(config.DispatcherRestartMaxDelay));
  syslog(LOG_INFO, "Metadata refresh interval %lu minutes",
         static_cast<unsigned long>(config.MetadataRefreshInterval));
  syslog(LOG_INFO, "Kafka socket timeout %lu seconds",
         static_cast<unsigned long>(config.KafkaSocketTimeout));
  syslog(LOG_INFO, "Pause rate limit initial %lu milliseconds",
         static_cast<unsigned long>(config.PauseRateLimitInitial));
  syslog(LOG_INFO, "Pause rate limit max double %lu",
         static_cast<unsigned long>(config.PauseRateLimitMaxDouble));
  syslog(LOG_INFO, "Minimum pause delay %lu milliseconds",
         static_cast<unsigned long>(config.MinPauseDelay));

  if (config.UseOldInputFormat) {
    syslog(LOG_INFO, "Omit timestamp from output: %s",
           config.OmitTimestamp ? "true" : "false");
  }

  syslog(LOG_INFO, "Discard reporting interval %lu seconds",
         static_cast<unsigned long>(config.DiscardReportInterval));
  syslog(LOG_INFO, "Debug directory [%s]", config.DebugDir.c_str());
  syslog(LOG_INFO, "Message debug time limit %lu seconds",
         static_cast<unsigned long>(config.MsgDebugTimeLimit));
  syslog(LOG_INFO, "Message debug byte limit %lu",
         static_cast<unsigned long>(config.MsgDebugByteLimit));
  syslog(LOG_INFO, "Compare metadata on refresh: %s",
         config.CompareMetadataOnRefresh ? "true" : "false");

  if (config.DiscardLogPath.empty()) {
    syslog(LOG_INFO, "Discard logfile creation is disabled");
  } else {
    syslog(LOG_INFO, "Discard logfile: [%s]", config.DiscardLogPath.c_str());
    syslog(LOG_INFO, "Discard log max file size: %lu kbytes",
           static_cast<unsigned long>(config.DiscardLogMaxFileSize));
    syslog(LOG_INFO, "Discard log max archive size: %lu kbytes",
           static_cast<unsigned long>(config.DiscardLogMaxArchiveSize));
    syslog(LOG_INFO, "Discard log bad msg prefix size: %lu bytes",
           static_cast<unsigned long>(config.DiscardLogBadMsgPrefixSize));
  }

  syslog(LOG_INFO, "Discard report bad msg prefix size: %lu bytes",
         static_cast<unsigned long>(config.DiscardReportBadMsgPrefixSize));
  syslog(LOG_INFO, "Using %s input datagram format",
         config.UseOldInputFormat ? "old" : "new");
  syslog(LOG_INFO, "Using %s output format",
         config.UseOldOutputFormat ? "old" : "new");
}