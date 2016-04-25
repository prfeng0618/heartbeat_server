int send_business_report_response(int fd, TREPORTRESP *preportRespMsg);
int recv_business_report_request(int fd, char *pbuff);
int send_business_issue_request(int fd, char *pbuff);
int recv_business_issue_response(int fd,char* pbuff);
int send_business_issue_response(int fd, TISSUERESP *pIssueResp);
int recv_business_issue_request(int fd,char* pbuff);





