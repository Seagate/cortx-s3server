using Amazon.IdentityManagement;
using Amazon.IdentityManagement.Model;
using Amazon.SecurityToken;
using Amazon.SecurityToken.Model;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

using CommandLine;
using CommandLine.Text;
using System.IO;
using System.Net;
using System.Xml;

namespace IAM
{
    class Options
    {
        [Option('x', "key", Required = false,
          HelpText = "AWS Key id")]
        public String KeyId { get; set; }

        [Option('y', "secret", Required = false,
          HelpText = "AWS secrey key")]
        public String SecretKey { get; set; }

        [Option('t', "token", Required = false,
          HelpText = "Federated User token")]
        public String Token { get; set; }

        [Option('a', "account", Required = false,
          HelpText = "Name of the account")]
        public String Account { get; set; }

        [Option('u', "user", Required = false,
          HelpText = "Name of the user")]
        public String UserName { get; set; }

        [Option('d', "duration", Required = false,
          HelpText = "Name of the user")]
        public String Duration { get; set; }

        [Option('k', "access-key", Required = false,
          HelpText = "access key id on which the action has to be performed")]
        public String UAccesskeyId { get; set; }

        [Option('n', "new-user", Required = false,
          HelpText = "New user name")]
        public String NewUserName { get; set; }

        [Option('s', "status", Required = false,
          HelpText = "Status of access key")]
        public String Status { get; set; }

        [Option('p', "path", Required = false,
          HelpText = "Status of access key")]
        public String Path { get; set; }

        [ValueList(typeof(List<string>), MaximumElements = 3)]
        public IList<string> CommandItems { get; set; }

        [HelpOption]
        public string GetUsage()
        {
            var help = new HelpText
            {
                AdditionalNewLineAfterOption = true,
                AddDashesToOption = true
            };
            help.AddPreOptionsLine("Usage:");
            help.AddPreOptionsLine("CreateAccount -a <Account Name>");
            help.AddPreOptionsLine("CreateAccessKey -u <User Name (Optional)>");
            help.AddPreOptionsLine("CreateUser -u <User Name> -p <Path (Optional)");
            help.AddPreOptionsLine("DeleteAccesskey -k <Access Key Id to be deleted>");
            help.AddPreOptionsLine("DeleteUser -u <User Name>");
            help.AddPreOptionsLine("GetFederationToken -u <User Name)> -d <Duration in seconds (Optional)>");
            help.AddPreOptionsLine("ListUsers");
            help.AddPreOptionsLine("ListAccessKeys -u <User Name (optional)");
            help.AddPreOptionsLine("UpdateUser -u <Old User Name> -p <New Path (Optional)> -n <New User Name>");
            help.AddPreOptionsLine("UpdateAccessKey -k <access key to be updated> -s <Active/Inactive> -u <User Name (Optional)>");
            help.AddOptions(this);
            return help;
        }
    }

    class Program
    { 
        static String AccessKeyId = "", SecretKey = "", Token = "";

        static AmazonIdentityManagementServiceConfig iamconfig = new AmazonIdentityManagementServiceConfig()
        {
            ServiceURL = "http://iam.seagate.com:8080",
            UseHttp = true,
            SignatureVersion = "4"
        };

        static AmazonSecurityTokenServiceConfig stsconfig = new AmazonSecurityTokenServiceConfig()
        {
            ServiceURL = "http://sts.seagate.com:8080",
            UseHttp = true,
            SignatureVersion = "4"
        };

        static AmazonIdentityManagementServiceClient iamClient;

        static void Main(string[] args)
        {
            var options = new Options();
            if (CommandLine.Parser.Default.ParseArguments(args, options))
            {
                if (String.Compare(options.CommandItems[0].ToLower(), "createaccount") != 0)
                {
                    if ((String.IsNullOrEmpty(options.KeyId) && !String.IsNullOrEmpty(options.SecretKey)) ||
                        (!String.IsNullOrEmpty(options.KeyId) && String.IsNullOrEmpty(options.SecretKey)))
                    {
                        Console.WriteLine("The credentials can be specified in the creds file" +
                            " or both AWS key id and secret key have to be given as command line arguments.");
                        Environment.Exit(1);
                    }

                    /* 
                        Try to fetch the default configuration from profile store
                        if the user doesn't specify the aws key id and aws access key
                        as command line arguments.
                    */

                    if (String.IsNullOrEmpty(options.KeyId))
                    {
                        Dictionary<string, string> creds = ParseAwsCredentials();
                        AccessKeyId = creds["aws_access_key_id"];
                        SecretKey = creds["aws_secret_access_key"];
                        if (creds.ContainsKey("token"))
                        {
                            Token = creds["token"];
                        }
                    }
                    else
                    {
                        AccessKeyId = options.KeyId;
                        SecretKey = options.SecretKey;
                        if (!String.IsNullOrEmpty(options.Token))
                        {
                            Token = options.Token;
                        }
                    }
                }

                switch (options.CommandItems[0].ToLower())
                {
                    case "createaccount":
                        if (String.IsNullOrEmpty(options.Account))
                        {
                            Console.WriteLine("Account name missing");
                            Environment.Exit(1);
                        }
                        CreateAccount(options.Account);
                        break;

                    case "createaccesskey":
                        CreateAccessKey(options.UserName);
                        break;

                    case "createuser":
                        if (String.IsNullOrEmpty(options.UserName))
                        {
                            Console.WriteLine("User name missing.");
                            Environment.Exit(1);
                        }

                        CreateUser(options.UserName, options.Path);
                        break;

                    case "deleteaccesskey":
                        if (String.IsNullOrEmpty(options.UAccesskeyId))
                        {
                            Console.WriteLine("Access Key id to be deleted isn't given.");
                            Environment.Exit(1);
                        }

                        if (String.IsNullOrEmpty(options.UserName))
                        {
                            DeleteAccessKey(options.UAccesskeyId);
                        }
                        else
                        {
                            DeleteAccessKey(options.UAccesskeyId, options.UserName);
                        }
                        break;

                    case "deleteuser":
                        if (String.IsNullOrEmpty(options.UserName))
                        {
                            Console.WriteLine("User name missing.");
                            Environment.Exit(1);
                        }

                        DeleteUser(options.UserName);
                        break;

                    case "getfederationtoken":
                        if (String.IsNullOrEmpty(options.UserName))
                        {
                            Console.WriteLine("User name missing.");
                            Environment.Exit(1);
                        }

                        if (String.IsNullOrEmpty(options.Duration))
                        {
                            GetFederationToken(options.UserName);
                        }
                        else
                        {
                            GetFederationToken(options.UserName, Int32.Parse(options.Duration));
                        }
                        break;

                    case "listusers":
                        ListUsers();
                        break;

                    case "listaccesskeys":
                        ListAccessKeys(options.UserName);
                        break;

                    case "updateuser":
                        if (String.IsNullOrEmpty(options.UserName))
                        {
                            Console.WriteLine("User name missing.");
                            Environment.Exit(1);
                        }

                        UpdateUser(options.UserName, options.NewUserName, options.Path);
                        break;

                    case "updateaccesskey":
                        if (String.IsNullOrEmpty(options.Status))
                        {
                            Console.WriteLine("Status is missing.");
                            Environment.Exit(1);
                        }

                        if (!(options.Status.Equals("Active") || options.Status.Equals("Inactive")))
                        {
                            Console.WriteLine("Invalid status");
                            Environment.Exit(1);
                        }

                        if (String.IsNullOrEmpty(options.UAccesskeyId))
                        {
                            Console.WriteLine("Give the access key id to be updated");
                            Environment.Exit(1);
                        }

                        if (String.IsNullOrEmpty(options.UserName))
                        {
                            UpdateAccessKey(options.UAccesskeyId, options.Status);
                        }
                        else
                        {
                            UpdateAccessKey(options.UAccesskeyId, options.Status, options.UserName);
                        }

                        break;

                    default:
                        Console.WriteLine("Incorrect format or operation not supported.");
                        Console.WriteLine("Use --help option to see the usage.");
                        break;
                }
            }

            Console.ReadLine();
        }

        private static string[] ParseRequest(string request)
        {
            if (!request.StartsWith("s3://"))
            {
                Console.WriteLine(
                    "Incorrect command. Use --help option to see the usage");
                Environment.Exit(1);
            }

            string[] temp = request.Split(new string[] { "//" }, StringSplitOptions.None);
            string[] request_components = temp[1].Split(new[] { "/" }, 2, StringSplitOptions.None);
            return request_components;
        }

        private static Dictionary<string, string> ParseAwsCredentials()
        {
            Dictionary<string, string> creds = new Dictionary<string, string>();
            string userProfile = Environment.GetFolderPath(Environment.SpecialFolder.UserProfile);
            string creds_file = Path.Combine(userProfile, ".aws", "credentials");

            if (!Directory.Exists(creds_file))
            {
                Console.WriteLine("Crendentials file missing. Provide the creds as command line arguments or create creds file.");
            }
            string[] temp;

            foreach (string line in File.ReadLines(creds_file))
            {
                temp = line.Split('=');
                creds.Add(temp[0], temp[1]);
            }

            return creds;
        }

        private static void CreateAccount(String AccountName)
        {
            WebRequest request = WebRequest.Create(iamconfig.ServiceURL);
            string postData = String.Format("Action=CreateAccount&AccountName={0}", AccountName);
            byte[] byteArray = Encoding.UTF8.GetBytes(postData);

            request.Method = "POST";
            request.ContentType = "application/x-www-form-urlencoded";
            request.ContentLength = byteArray.Length;

            Stream dataStream = request.GetRequestStream();
            dataStream.Write(byteArray, 0, byteArray.Length);
            dataStream.Close();

            try
            {
                WebResponse response = request.GetResponse();
                dataStream = response.GetResponseStream();

                StreamReader reader = new StreamReader(dataStream);
                string responseFromServer = reader.ReadToEnd();
                CreateAccountResponseParser(responseFromServer);

                reader.Close();
                dataStream.Close();
                response.Close();
            }
            catch (WebException ex)
            {
                Console.WriteLine("Account exists: " + ex.ToString());
            }
        }

        private static void CreateAccountResponseParser(String reponse)
        {
            StringBuilder output = new StringBuilder();

            using (XmlReader reader = XmlReader.Create(new StringReader(reponse)))
            {
                reader.ReadToFollowing("RootUserName");
                output.AppendLine("Root User: " + reader.ReadElementContentAsString());
                output.AppendLine("Access Key Id: " + reader.ReadElementContentAsString());
                output.AppendLine("Status: " + reader.ReadElementContentAsString());
                output.AppendLine("Secret Key: " + reader.ReadElementContentAsString());

            }

            Console.Write(output.ToString());
        }

        private static void CreateUser(String User, String Path)
        {
            if (String.IsNullOrEmpty(Token))
            {
                iamClient = new AmazonIdentityManagementServiceClient(AccessKeyId, SecretKey, iamconfig);
            }
            else
            {
                iamClient = new AmazonIdentityManagementServiceClient(AccessKeyId, SecretKey, Token, iamconfig);
            }

            CreateUserRequest req = new CreateUserRequest(User);
            if (!String.IsNullOrEmpty(Path))
            {
                req.Path = Path;
            }
            try
            {
                CreateUserResponse response = iamClient.CreateUser(req);
                Console.WriteLine("User created");
            }
            catch (Exception ex)
            {
                Console.WriteLine("Error occured while creating user. " + ex.ToString());
            }
        }

        private static void CreateAccessKey(String User)
        {
            if (String.IsNullOrEmpty(Token))
            {
                iamClient = new AmazonIdentityManagementServiceClient(AccessKeyId, SecretKey, iamconfig);
            }
            else
            {
                iamClient = new AmazonIdentityManagementServiceClient(AccessKeyId, SecretKey, Token, iamconfig);
            }

            try
            {
                CreateAccessKeyRequest accesskeyReq = new CreateAccessKeyRequest();
                if (!String.IsNullOrEmpty(User))
                {
                    accesskeyReq.UserName = User;
                }
                CreateAccessKeyResponse response = iamClient.CreateAccessKey(accesskeyReq);
                Console.WriteLine("Access keys :{0}, Secret Key: {1}", response.AccessKey.AccessKeyId,
                    response.AccessKey.SecretAccessKey);
            }
            catch (Exception ex)
            {
                Console.WriteLine("Error occured while creating user. " + ex.ToString());
            }
        }

        private static void DeleteAccessKey(String DelAccessKeyid, String User = "")
        {
            if (String.IsNullOrEmpty(Token))
            {
                iamClient = new AmazonIdentityManagementServiceClient(AccessKeyId, SecretKey, iamconfig);
            }
            else
            {
                iamClient = new AmazonIdentityManagementServiceClient(AccessKeyId, SecretKey, Token, iamconfig);
            }

            try
            {
                DeleteAccessKeyRequest accesskeyReq = new DeleteAccessKeyRequest(DelAccessKeyid);
                if (!String.IsNullOrEmpty(User))
                {
                    accesskeyReq.UserName = User;
                }
                DeleteAccessKeyResponse response = iamClient.DeleteAccessKey(accesskeyReq);
                Console.WriteLine("Access Key deleted successfully.");
            }
            catch (Exception ex)
            {
                Console.WriteLine("Error occured while creating user. " + ex.ToString());
            }
        }

        private static void DeleteUser(String UserName)
        {
            if (String.IsNullOrEmpty(Token))
            {
                iamClient = new AmazonIdentityManagementServiceClient(AccessKeyId, SecretKey, iamconfig);
            }
            else
            {
                iamClient = new AmazonIdentityManagementServiceClient(AccessKeyId, SecretKey, Token, iamconfig);
            }

            DeleteUserRequest req = new DeleteUserRequest(UserName);

            try
            {
                DeleteUserResponse response = iamClient.DeleteUser(req);
                Console.WriteLine("User Deleted");
            }
            catch (Exception ex)
            {
                Console.WriteLine("Error occured while creating user. " + ex.ToString());
            }
        }

        private static void GetFederationToken(String User, int Duration = -1)
        {
            AmazonSecurityTokenServiceClient stsClient;
            if (String.IsNullOrEmpty(Token))
            {
                stsClient = new AmazonSecurityTokenServiceClient(AccessKeyId, SecretKey, stsconfig);
            }
            else
            {
                stsClient = new AmazonSecurityTokenServiceClient(AccessKeyId, SecretKey, Token, stsconfig);
            }

            GetFederationTokenRequest federationTokenRequest = new GetFederationTokenRequest();
            if (!String.IsNullOrEmpty(User))
            {
                federationTokenRequest.Name = User;
            }

            if(Duration != -1)
            {
                federationTokenRequest.DurationSeconds = Duration;
            }


            GetFederationTokenResponse federationTokenResponse = stsClient.GetFederationToken(federationTokenRequest);
            Console.WriteLine("acess key id: {0}", federationTokenResponse.Credentials.AccessKeyId);
            Console.WriteLine("secret key: {0}", federationTokenResponse.Credentials.SecretAccessKey);
            Console.WriteLine("session token: {0}", federationTokenResponse.Credentials.SessionToken);
            Console.WriteLine("Expiration: {0}", federationTokenResponse.Credentials.Expiration);
        }

        private static void ListUsers()
        {
            if (String.IsNullOrEmpty(Token))
            {
                iamClient = new AmazonIdentityManagementServiceClient(AccessKeyId, SecretKey, iamconfig);
            }
            else
            {
                iamClient = new AmazonIdentityManagementServiceClient(AccessKeyId, SecretKey, Token, iamconfig);
            }

            ListUsersRequest req = new ListUsersRequest();
            try
            {
                ListUsersResponse response = iamClient.ListUsers(req);
                foreach (User u in response.Users)
                {
                    Console.WriteLine("Id: {0}, User name : {1}, Path: {2}, Create Date: {3}", u.UserId, u.UserName, u.Path, u.CreateDate.ToString());
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine("Error occured while creating user. " + ex.ToString());
            }
        }

        private static void ListAccessKeys(String UserName)
        {
            if (String.IsNullOrEmpty(Token))
            {
                iamClient = new AmazonIdentityManagementServiceClient(AccessKeyId, SecretKey, iamconfig);
            }
            else
            {
                iamClient = new AmazonIdentityManagementServiceClient(AccessKeyId, SecretKey, Token, iamconfig);
            }

            ListAccessKeysRequest req = new ListAccessKeysRequest();
            if (!String.IsNullOrEmpty(UserName))
            {
                req.UserName = UserName;   
            }

            try
            {
                ListAccessKeysResponse response = iamClient.ListAccessKeys(req);
                foreach(AccessKeyMetadata a in response.AccessKeyMetadata)
                {
                    Console.WriteLine("User Name: {0}, Access Key Id: {1}, Status: {2}, Create Date: {3}", a.UserName, a.AccessKeyId, a.Status, a.CreateDate);
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine("Error occured while creating user. " + ex.ToString());
            }
        }

        private static void UpdateUser(String OldUserName, String NewUserName, String NewPath)
        {
            if (String.IsNullOrEmpty(Token))
            {
                iamClient = new AmazonIdentityManagementServiceClient(AccessKeyId, SecretKey, iamconfig);
            }
            else
            {
                iamClient = new AmazonIdentityManagementServiceClient(AccessKeyId, SecretKey, Token, iamconfig);
            }

            UpdateUserRequest req = new UpdateUserRequest(OldUserName);
            if (!String.IsNullOrEmpty(NewUserName))
            {
                req.NewUserName = NewUserName;
            }

            if (!String.IsNullOrEmpty(NewPath))
            {
                req.NewPath = NewPath;
            }

            try
            {
                UpdateUserResponse response = iamClient.UpdateUser(req);
                Console.WriteLine("User updated");
            }
            catch (Exception ex)
            {
                Console.WriteLine("Error occured while creating user. " + ex.ToString());
            }
        }

        private static void UpdateAccessKey(String UAccessKeyId, String Status, String UserName = "")
        {
            if (String.IsNullOrEmpty(Token))
            {
                iamClient = new AmazonIdentityManagementServiceClient(AccessKeyId, SecretKey, iamconfig);
            }
            else
            {
                iamClient = new AmazonIdentityManagementServiceClient(AccessKeyId, SecretKey, Token, iamconfig);
            }

            UpdateAccessKeyRequest req = new UpdateAccessKeyRequest(UAccessKeyId, Status);
            if (!String.IsNullOrEmpty(UserName))
            {
                req.UserName = UserName;
            }

            try
            {
                UpdateAccessKeyResponse response = iamClient.UpdateAccessKey(req);
                Console.WriteLine("User updated");
            }
            catch (Exception ex)
            {
                Console.WriteLine("Error occured while creating user. " + ex.ToString());
            }
        }
    }
}
