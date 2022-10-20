#!/bin/bash
# primihub deploy script

# First, install docker and docker-compose

centos(){
    iptables -F
    systemctl stop firewalld
    systemctl disable firewalld
    setenforce 0
    sed -i s#SELINUX=enforcing#SELINUX=disabled#  /etc/selinux/config
    docker version
    if [ $? -eq 0 ];
    then
        echo "docker installed"
    else
        wget -O /etc/yum.repos.d/docker-ce.repo https://mirrors.aliyun.com/docker-ce/linux/centos/docker-ce.repo
        yum -y install docker-ce device-mapper-persistent-data lvm2
        systemctl daemon-reload
        systemctl start docker && systemctl enable docker
        echo "docker install succeed !"
    fi
}

ubuntu(){
    docker version
    if [ $? -eq 0 ];
    then
        echo "docker installed"
    else
        apt-get update
        apt-get -y install apt-transport-https ca-certificates curl software-properties-common
        curl -fsSL https://mirrors.aliyun.com/docker-ce/linux/ubuntu/gpg | sudo apt-key add -
        add-apt-repository "deb [arch=amd64] https://mirrors.aliyun.com/docker-ce/linux/ubuntu $(lsb_release -cs) stable"
        apt-get -y update
        apt-get -y install docker-ce
        echo "docker install succeed !"
    fi
}

if [ $(uname -s) == "Linux" ];
then
  grep "Ubuntu" /etc/issue >> /dev/null
  if [ $? -eq 0 ];
  then
      ubuntu
  else
      centos
  fi
elif [ $(uname -s) == "Darwin" ]; then
  which docker-compose > /dev/null
  if [ $? != 0 ];
  then
    echo "Cannot find docker compose, please install it first."
    echo "Read the official document from https://docs.docker.com/compose/install/"
    exit 1
  fi
else
  echo "not support yet"
  exit 1
fi

docker-compose version
if [ $? -eq 0 ];
then
    echo "docker-compose installed"
else
    curl -L "https://github.com/docker/compose/releases/download/v2.2.2/docker-compose-$(uname -s)-$(uname -m)" -o /usr/bin/docker-compose
    if [ $? -eq 0 ];
    then
        chmod +x /usr/bin/docker-compose
        echo "docker-compose install succeed !"
    else
        echo "Download docker-compose failed!"
        exit
    fi
fi

# Pull all the necessary images to avoid pulling multiple times
for i in `cat .env | cut -d '=' -f 2`
do
    docker pull $i
done

# Finally, start the application
docker-compose up -d
