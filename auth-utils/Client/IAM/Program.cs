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

        [Option('n', "name", Required = false,
          HelpText = "Name/ ARN")]
        public String Name { get; set; }

        [Option('s', "status", Required = false,
          HelpText = "Status of access key")]
        public String Status { get; set; }

        [Option('p', "path", Required = false,
          HelpText = "Status of access key")]
        public String Path { get; set; }

        [Option('f', "file", Required = false,
          HelpText = "Input file")]
        public String File { get; set; }

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
            help.AddPreOptionsLine("CreateSAMLProvider -n <Name of saml provider> -f <Path of metadata file>");
            help.AddPreOptionsLine("DeleteAccesskey -k <Access Key Id to be deleted>");
            help.AddPreOptionsLine("DeleteUser -u <User Name>");
            help.AddPreOptionsLine("Delete Saml Provider -n <Saml Provider ARN>");
            help.AddPreOptionsLine("GetFederationToken -u <User Name)> -d <Duration in seconds (Optional)>");
            help.AddPreOptionsLine("ListUsers");
            help.AddPreOptionsLine("ListAccessKeys -u <User Name (optional)");
            help.AddPreOptionsLine("ListSamlProviders");
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
            SignatureVersion = "4",
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

                    case "createsamlprovider":
                        if (String.IsNullOrEmpty(options.Name))
                        {
                            Console.WriteLine("Give a name for saml provider.");
                            Environment.Exit(1);
                        }
                        if (String.IsNullOrEmpty(options.File))
                        {
                            Console.WriteLine("Metadata file is required.");
                            Environment.Exit(1);
                        }

                        CreateSAMLProvider(options.Name, options.File);
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

                    case "deletesamlprovider":
                        if (String.IsNullOrEmpty(options.Name))
                        {
                            Console.WriteLine("ARN of provider isn't given.");
                            Environment.Exit(1);
                        }

                        DeleteSAMLProvider(options.Name);
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

                    case "listsamlproviders":
                        ListSamlProviders();
                        break;

                    case "updateuser":
                        if (String.IsNullOrEmpty(options.UserName))
                        {
                            Console.WriteLine("User name missing.");
                            Environment.Exit(1);
                        }

                        UpdateUser(options.UserName, options.Name, options.Path);
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

                    case "assumerolewithsaml":
                        AssumeRoleWithSaml();
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

        private static void CreateSAMLProvider(String Name, String MetadataFile)
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
                CreateSAMLProviderRequest Req = new CreateSAMLProviderRequest();
                if (File.Exists(MetadataFile))
                {
                    String Metadata = File.ReadAllText(MetadataFile);
                    Req.SAMLMetadataDocument = Metadata;
                    Req.Name = Name;


                    CreateSAMLProviderResponse response = iamClient.CreateSAMLProvider(Req);
                    Console.WriteLine("Saml Provider Created successfully.");
                }
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

        private static void DeleteSAMLProvider(String Name)
        {
            if (String.IsNullOrEmpty(Token))
            {
                iamClient = new AmazonIdentityManagementServiceClient(AccessKeyId, SecretKey);
                //iamClient = new AmazonIdentityManagementServiceClient(AccessKeyId, SecretKey, iamconfig);
            }
            else
            {
                iamClient = new AmazonIdentityManagementServiceClient(AccessKeyId, SecretKey, Token, iamconfig);
            }

            try
            {
                DeleteSAMLProviderRequest Req = new DeleteSAMLProviderRequest();
                Req.SAMLProviderArn = Name;

                DeleteSAMLProviderResponse response = iamClient.DeleteSAMLProvider(Req);
                Console.WriteLine("Saml Provider deleted successfully.");
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

            if (Duration != -1)
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
                foreach (AccessKeyMetadata a in response.AccessKeyMetadata)
                {
                    Console.WriteLine("User Name: {0}, Access Key Id: {1}, Status: {2}, Create Date: {3}", a.UserName, a.AccessKeyId, a.Status, a.CreateDate);
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine("Error occured while creating user. " + ex.ToString());
            }
        }

        private static void ListSamlProviders()
        {
            if (String.IsNullOrEmpty(Token))
            {
                iamClient = new AmazonIdentityManagementServiceClient(AccessKeyId, SecretKey);
                //iamClient = new AmazonIdentityManagementServiceClient(AccessKeyId, SecretKey, iamconfig);
            }
            else
            {
                iamClient = new AmazonIdentityManagementServiceClient(AccessKeyId, SecretKey, Token, iamconfig);
            }

            ListSAMLProvidersRequest req = new ListSAMLProvidersRequest();
            try
            {
                ListSAMLProvidersResponse response = iamClient.ListSAMLProviders(req);
                foreach (SAMLProviderListEntry entry in response.SAMLProviderList)
                {
                    Console.WriteLine("ARN: {0}, Valid Until: {1}, Create Date: {2}", entry.Arn, entry.ValidUntil, entry.CreateDate);
                }
                Console.WriteLine();
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

        private static void AssumeRoleWithSaml()
        {
            AmazonSecurityTokenServiceClient stsClient = new AmazonSecurityTokenServiceClient(AccessKeyId, SecretKey);
            AssumeRoleWithSAMLRequest req = new AssumeRoleWithSAMLRequest();
            req.PrincipalArn = "arn:aws:iam::842327222694:saml-provider/ADFS";
            req.RoleArn = "arn:aws:iam::842327222694:role/ADFS-Dev";
            req.SAMLAssertion = "PHNhbWxwOlJlc3BvbnNlIElEPSJfN2RkZjRjMTMtODdkNC00NDkxLWE5NTAtNDlmZGYxOTdhYTAzIiBWZXJzaW9uPSIyLjAiIElzc3VlSW5zdGFudD0iMjAxNS0xMC0wOFQyMTo0MjowNi4xMDZaIiBEZXN0aW5hdGlvbj0iaHR0cHM6Ly9zaWduaW4uYXdzLmFtYXpvbi5jb20vc2FtbCIgQ29uc2VudD0idXJuOm9hc2lzOm5hbWVzOnRjOlNBTUw6Mi4wOmNvbnNlbnQ6dW5zcGVjaWZpZWQiIHhtbG5zOnNhbWxwPSJ1cm46b2FzaXM6bmFtZXM6dGM6U0FNTDoyLjA6cHJvdG9jb2wiPjxJc3N1ZXIgeG1sbnM9InVybjpvYXNpczpuYW1lczp0YzpTQU1MOjIuMDphc3NlcnRpb24iPmh0dHA6Ly9XSU4tRkpIRjRPTjBHMTgudGVzdC5vcmcvYWRmcy9zZXJ2aWNlcy90cnVzdDwvSXNzdWVyPjxzYW1scDpTdGF0dXM+PHNhbWxwOlN0YXR1c0NvZGUgVmFsdWU9InVybjpvYXNpczpuYW1lczp0YzpTQU1MOjIuMDpzdGF0dXM6U3VjY2VzcyIgLz48L3NhbWxwOlN0YXR1cz48QXNzZXJ0aW9uIElEPSJfYjIyZmE4NTgtMzMzMS00YzU2LTlmZjktYjhmNzgzZDExOTZhIiBJc3N1ZUluc3RhbnQ9IjIwMTUtMTAtMDhUMjE6NDI6MDYuMTA2WiIgVmVyc2lvbj0iMi4wIiB4bWxucz0idXJuOm9hc2lzOm5hbWVzOnRjOlNBTUw6Mi4wOmFzc2VydGlvbiI+PElzc3Vlcj5odHRwOi8vV0lOLUZKSEY0T04wRzE4LnRlc3Qub3JnL2FkZnMvc2VydmljZXMvdHJ1c3Q8L0lzc3Vlcj48ZHM6U2lnbmF0dXJlIHhtbG5zOmRzPSJodHRwOi8vd3d3LnczLm9yZy8yMDAwLzA5L3htbGRzaWcjIj48ZHM6U2lnbmVkSW5mbz48ZHM6Q2Fub25pY2FsaXphdGlvbk1ldGhvZCBBbGdvcml0aG09Imh0dHA6Ly93d3cudzMub3JnLzIwMDEvMTAveG1sLWV4Yy1jMTRuIyIgLz48ZHM6U2lnbmF0dXJlTWV0aG9kIEFsZ29yaXRobT0iaHR0cDovL3d3dy53My5vcmcvMjAwMS8wNC94bWxkc2lnLW1vcmUjcnNhLXNoYTI1NiIgLz48ZHM6UmVmZXJlbmNlIFVSST0iI19iMjJmYTg1OC0zMzMxLTRjNTYtOWZmOS1iOGY3ODNkMTE5NmEiPjxkczpUcmFuc2Zvcm1zPjxkczpUcmFuc2Zvcm0gQWxnb3JpdGhtPSJodHRwOi8vd3d3LnczLm9yZy8yMDAwLzA5L3htbGRzaWcjZW52ZWxvcGVkLXNpZ25hdHVyZSIgLz48ZHM6VHJhbnNmb3JtIEFsZ29yaXRobT0iaHR0cDovL3d3dy53My5vcmcvMjAwMS8xMC94bWwtZXhjLWMxNG4jIiAvPjwvZHM6VHJhbnNmb3Jtcz48ZHM6RGlnZXN0TWV0aG9kIEFsZ29yaXRobT0iaHR0cDovL3d3dy53My5vcmcvMjAwMS8wNC94bWxlbmMjc2hhMjU2IiAvPjxkczpEaWdlc3RWYWx1ZT56Uk5pOUdBOVFiUUFpWkROMU1XckpCTjlqSlpFSzM5Qm5CbzZSK0Z6b0pZPTwvZHM6RGlnZXN0VmFsdWU+PC9kczpSZWZlcmVuY2U+PC9kczpTaWduZWRJbmZvPjxkczpTaWduYXR1cmVWYWx1ZT56TGV2ejc1QXJ6aWVKd09CWkFFS0VEY1AxVE1tSnNDdFlOSmNWY3hjMUk2aWtjb2FWVDVPTEdISm9JMXY1bDFvZnJGcHRjWVVSaEpXQ3RXV05sVWFveUpPUnFRdjRNY2dVdHZzTjkrdUZEYXhrMHZmYnNtWkNRaVdpc1pQVGN1di9OSHNOUmNxV2FBSVdIdmRDK0w3cUVDREp3ZUdJWk02a2dJdjd0NFVVTE1nRFVxbi9xbm43dlg0UWYxMmk3YlJtdTRELzJnZENkdHpRaUtWK2EyM1JtaGQrSTBRSGNvWjNacmc4a0NaUVdZMUlYekVvWTdXTGUvNVNYcFhNV1lJTlR6d2tEdFlITmJvbEhja01NUnA3NkRNSXFPaTA4cmNueVhRc0VtZjdrM1FFYUM5MUd5MUVBdU0rQnhzMmw1RktMSG8yOEZzT2xadERRNllYenM0VEE9PTwvZHM6U2lnbmF0dXJlVmFsdWU+PEtleUluZm8geG1sbnM9Imh0dHA6Ly93d3cudzMub3JnLzIwMDAvMDkveG1sZHNpZyMiPjxkczpYNTA5RGF0YT48ZHM6WDUwOUNlcnRpZmljYXRlPk1JSUM3RENDQWRTZ0F3SUJBZ0lRZERPK3NZL1VJWU5FRFBmNW85Vkt2REFOQmdrcWhraUc5dzBCQVFzRkFEQXlNVEF3TGdZRFZRUURFeWRCUkVaVElGTnBaMjVwYm1jZ0xTQlhTVTR0UmtwSVJqUlBUakJITVRndWRHVnpkQzV2Y21jd0hoY05NVFV4TURBMk1qTXlOakU0V2hjTk1UWXhNREExTWpNeU5qRTRXakF5TVRBd0xnWURWUVFERXlkQlJFWlRJRk5wWjI1cGJtY2dMU0JYU1U0dFJrcElSalJQVGpCSE1UZ3VkR1Z6ZEM1dmNtY3dnZ0VpTUEwR0NTcUdTSWIzRFFFQkFRVUFBNElCRHdBd2dnRUtBb0lCQVFEdWU0NjV1L3ZERGlHbFhTLytPTVE5Vjc3Z1l3Z0R4OGw3RFk2aVFMM2lJVENsVzBqeUR5OE8wQzl6b1ZNeGR3cDNncXZYMGFxZEUrRFVLUjBzWG11THN5M1ZIdi9iZ2JKN0Z1RE91TVZ6TjRZMXozc2t1QktYNEwrM20yanl5VFhaR09hQWtMdEJ3c1FMSWdjS1kxZm5iR3VDdjM1Y2lneS9ma1ppdnhmaHhLYXRHUFNXRVhxWGVQN0VaMFlyNy9mYnZTRkJKUVhyM3FMeVlmcWtmdlFBd2xUcGd4RzR1SGEwYmVCNkpuWXFPQXJPczJrc2RWbVdtODdYeWJhTVMvcmZzbWQ2c3QvdzRzeVFhYlpGb0duUlRFdmwvMG4zUWt2M21NUU4vRzJqemFLMjluY0ZkazhGaE9qT05kb0gxZkQyd2NCejlPcGlpb2ZQd0lLSVFad2xBZ01CQUFFd0RRWUpLb1pJaHZjTkFRRUxCUUFEZ2dFQkFJVkYvMzlwRWNhVU1kaE9mNmYyYXpySmNvM3FYWlEySWJhajdleUtXaE9nQU5tWEN5NnBacVdLb0FaeEpZdEN5WXlFUGZNL0YwRVRSWXJCQk9WQVRiSEdYSXV4VVhsYUZJNW5qa0FudEpRdy9KbVdVemNsR3hTdjJNNFliZWpRbk9RN1ZoZ0tjaDV3NDlJUHIvbDNHYXlQMVFiOU5ObHVMK3J5UnpHRHd3VjR5ZTVzUG5VN2FqMnlLNU9lYU11N2RmL1FwY1dUUU1zUVNtTXl0MWxIM0E4NGdtN2hwUCtOSmgrRHhjSXJ2My9QVzZRa3E4UnZKZllad1hIVjB0VmtSUyt2SDJaWFBmUnFkN3g2UTQwOFRTSnpHb3JqakFrelpZam1xRmlNNGdPN0ptOURIZUljQXNmSGVCcGpJMWZvWlU5WnU1dGI5S280Q0JqVFVybEVLZU09PC9kczpYNTA5Q2VydGlmaWNhdGU+PC9kczpYNTA5RGF0YT48L0tleUluZm8+PC9kczpTaWduYXR1cmU+PFN1YmplY3Q+PE5hbWVJRCBGb3JtYXQ9InVybjpvYXNpczpuYW1lczp0YzpTQU1MOjIuMDpuYW1laWQtZm9ybWF0OnBlcnNpc3RlbnQiPlRFU1RcQWRtaW5pc3RyYXRvcjwvTmFtZUlEPjxTdWJqZWN0Q29uZmlybWF0aW9uIE1ldGhvZD0idXJuOm9hc2lzOm5hbWVzOnRjOlNBTUw6Mi4wOmNtOmJlYXJlciI+PFN1YmplY3RDb25maXJtYXRpb25EYXRhIE5vdE9uT3JBZnRlcj0iMjAxNS0xMC0wOFQyMTo0NzowNi4xMDZaIiBSZWNpcGllbnQ9Imh0dHBzOi8vc2lnbmluLmF3cy5hbWF6b24uY29tL3NhbWwiIC8+PC9TdWJqZWN0Q29uZmlybWF0aW9uPjwvU3ViamVjdD48Q29uZGl0aW9ucyBOb3RCZWZvcmU9IjIwMTUtMTAtMDhUMjE6NDI6MDYuMTA2WiIgTm90T25PckFmdGVyPSIyMDE1LTEwLTA4VDIyOjQyOjA2LjEwNloiPjxBdWRpZW5jZVJlc3RyaWN0aW9uPjxBdWRpZW5jZT51cm46YW1hem9uOndlYnNlcnZpY2VzPC9BdWRpZW5jZT48L0F1ZGllbmNlUmVzdHJpY3Rpb24+PC9Db25kaXRpb25zPjxBdHRyaWJ1dGVTdGF0ZW1lbnQ+PEF0dHJpYnV0ZSBOYW1lPSJodHRwczovL2F3cy5hbWF6b24uY29tL1NBTUwvQXR0cmlidXRlcy9Sb2xlIj48QXR0cmlidXRlVmFsdWU+YXJuOmF3czppYW06Ojg0MjMyNzIyMjY5NDpzYW1sLXByb3ZpZGVyL0FERlMsYXJuOmF3czppYW06Ojg0MjMyNzIyMjY5NDpyb2xlL0FERlMtRGV2PC9BdHRyaWJ1dGVWYWx1ZT48QXR0cmlidXRlVmFsdWU+YXJuOmF3czppYW06Ojg0MjMyNzIyMjY5NDpzYW1sLXByb3ZpZGVyL0FERlMsYXJuOmF3czppYW06Ojg0MjMyNzIyMjY5NDpyb2xlL0FERlMtUHJvZHVjdGlvbjwvQXR0cmlidXRlVmFsdWU+PC9BdHRyaWJ1dGU+PEF0dHJpYnV0ZSBOYW1lPSJodHRwczovL2F3cy5hbWF6b24uY29tL1NBTUwvQXR0cmlidXRlcy9Sb2xlU2Vzc2lvbk5hbWUiPjxBdHRyaWJ1dGVWYWx1ZT5hZG1pQHRlc3Qub3JnPC9BdHRyaWJ1dGVWYWx1ZT48L0F0dHJpYnV0ZT48L0F0dHJpYnV0ZVN0YXRlbWVudD48QXV0aG5TdGF0ZW1lbnQgQXV0aG5JbnN0YW50PSIyMDE1LTEwLTA4VDIxOjA5OjUzLjQ3MVoiIFNlc3Npb25JbmRleD0iX2IyMmZhODU4LTMzMzEtNGM1Ni05ZmY5LWI4Zjc4M2QxMTk2YSI+PEF1dGhuQ29udGV4dD48QXV0aG5Db250ZXh0Q2xhc3NSZWY+dXJuOmZlZGVyYXRpb246YXV0aGVudGljYXRpb246d2luZG93czwvQXV0aG5Db250ZXh0Q2xhc3NSZWY+PC9BdXRobkNvbnRleHQ+PC9BdXRoblN0YXRlbWVudD48L0Fzc2VydGlvbj48L3NhbWxwOlJlc3BvbnNlPg==";

            AssumeRoleWithSAMLResponse response = stsClient.AssumeRoleWithSAML(req);
            Console.ReadLine();
        }
    }
}
